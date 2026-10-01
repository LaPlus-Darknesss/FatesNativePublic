#include "fates/event/native_phase_event_vm.hpp"
#include "fates/cmvm/cmvm.hpp"
#include "fates/detail/cmvm_runtime.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace fates::event::native {
namespace {
using namespace fates::detail::cmvm;
enum class WordKind : std::uint8_t { Unknown, Integer, LocalAddress, ArchiveAddress, FlagBitsAddress };
struct WordTag {
    WordKind kind{WordKind::Unknown};
    std::uint64_t frame{};
    std::shared_ptr<const PhaseEventArchive> archive;
    runtime::native::EventFlagResult flag_bits;
};
std::uint32_t U32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1])<<8u) |
        (std::uint32_t(bytes[at+2])<<16u) | (std::uint32_t(bytes[at+3])<<24u);
}
std::int16_t I16(std::span<const std::uint8_t> bytes, std::size_t at) {
    return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(unsigned(bytes[at])*256u+bytes[at+1]));
}
bool Fits(std::size_t at, std::size_t length, std::size_t size) {
    return at<=size && length<=size-at;
}
}
struct PhaseEventVm::State {
    struct Frame {
        cmvm::native::ScriptFunctionRef reference;
        cmvm::CmArchiveView archive;
        cmvm::CmFunctionView function;
        std::size_t base{}, floor{};
        std::uint64_t identity{};
        cmvm::native::ScriptAttachment attachment;
    };
    PhaseEventSelection selected;
    cmvm::native::ScriptFunctionRef root;
    cmvm::native::ScriptAttachment root_attachment;
    std::shared_ptr<const cmvm::native::ScriptFunctionRegistry> registry;
    std::shared_ptr<const NativePhaseEventQueries> queries;
    std::shared_ptr<const NativeEventFlagCommands> flags;
    std::shared_ptr<const NativeUnitEventQueries> units;
    std::shared_ptr<map::native::NativeCameraWait> camera;
    std::shared_ptr<NativeEventCamera> camera_commands;
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session;
    // Individually allocated frames keep saved CmFunctionView pointers stable.
    std::vector<std::unique_ptr<Frame>> frames;
    std::uint64_t next_frame{1};
    cmvm::CmContextState context;
    std::vector<cmvm::VmWord> stack;
    std::vector<WordTag> tags;
    PhaseVmObservation observation;

    static std::unique_ptr<Frame> MakeFrame(const cmvm::native::ScriptFunctionRef& reference,
        std::size_t base_index, std::uint64_t identity) {
        auto frame=std::make_unique<Frame>();
        frame->reference=reference;frame->base=base_index;frame->identity=identity;
        const auto& info=*reference.info();
        const auto bytes=reference.table()->archive()->bytes();
        const auto* base=reinterpret_cast<const std::byte*>(bytes.data());
        frame->archive={base,bytes.size(),base+U32(bytes,0x20)};
        frame->function={base+info.code_offset,info.type,info.argument_count,info.local_words,&frame->archive,false};
        frame->floor=base_index+info.local_words+3;
        return frame;
    }
    PhaseVmStatus LocalCell(const WordTag& tag, cmvm::VmWord value, std::size_t& index) const {
        if (tag.kind!=WordKind::LocalAddress) return PhaseVmStatus::InvalidValue;
        const Frame* owner=nullptr;
        for (const auto& frame:frames) if (frame->identity==tag.frame) {owner=frame.get();break;}
        if (!owner) return PhaseVmStatus::DanglingLocal;
        const auto address=static_cast<std::uintptr_t>(value);
        const auto base=reinterpret_cast<std::uintptr_t>(stack.data());
        if (address<base || (address-base)%sizeof(cmvm::VmWord)!=0) return PhaseVmStatus::InvalidLocal;
        index=(address-base)/sizeof(cmvm::VmWord);
        if (index<owner->base || index-owner->base>=owner->function.local_words) return PhaseVmStatus::InvalidLocal;
        return PhaseVmStatus::Ready;
    }
    PhaseVmStatus CheckValue(const WordTag& tag, cmvm::VmWord value) const {
        if (tag.kind==WordKind::Unknown) return PhaseVmStatus::UnknownLocal;
        if (tag.kind==WordKind::LocalAddress) {std::size_t index{};return LocalCell(tag,value,index);}
        if (tag.kind==WordKind::ArchiveAddress) {
            if (!tag.archive) return PhaseVmStatus::InvalidValue;
            const auto bytes=tag.archive->bytes();
            const auto address=static_cast<std::uintptr_t>(value);
            const auto base=reinterpret_cast<std::uintptr_t>(bytes.data());
            if (address<base || address-base>=bytes.size()) return PhaseVmStatus::InvalidValue;
        }
        if(tag.kind==WordKind::FlagBitsAddress) {
            const auto address=tag.flag_bits.address();
            if(!address)return PhaseVmStatus::StaleNativeContext;
            if(*address!=static_cast<std::uintptr_t>(value))return PhaseVmStatus::InvalidValue;
        }
        return PhaseVmStatus::Ready;
    }
    void Observe() {
        auto& o=observation;
        o.frame_depth=frames.size();
        o.current_function=frames.empty()?cmvm::native::ScriptFunctionRef{}:frames.back()->reference;
        o.evaluation_words=frames.empty()?0:static_cast<std::size_t>(context.stack_top-stack.data())-frames.back()->floor;
        if (!frames.empty() && context.instruction)
            o.code_offset=static_cast<std::uint32_t>(context.instruction-frames.back()->archive.base);
    }
    bool SessionValid() const noexcept {
        if(!session)return true;
        if(session->retired())return false;
        if(root_attachment && !session->IsAttached(root_attachment))return false;
        for(const auto& frame:frames)if(!session->IsAttached(frame->attachment))return false;
        return true;
    }
    PhaseVmStatus Enter(const cmvm::native::ScriptFunctionRef& target, std::size_t top, std::size_t depth,
        cmvm::native::ScriptAttachment attachment={}) {
        if (!target) return PhaseVmStatus::InvalidCall;
        if(session && (!session->IsAttached(attachment) || attachment.table()->archive()!=target.table()->archive()))
            return PhaseVmStatus::StaleScriptSession;
        const auto& info=*target.info();
        const auto argc=info.type==0?std::size_t(info.argument_count):0;
        // Retail reads a signed byte for type0. Reject malformed backwards frames.
        if (argc>=128 || argc>info.local_words) return PhaseVmStatus::InvalidCall;
        if (depth<argc) return PhaseVmStatus::StackUnderflow;
        const auto base=top-argc;
        if (std::size_t(info.local_words)+3>stack.size()-base) return PhaseVmStatus::StackLimit;
        if (frames.size()>=256 || next_frame==std::numeric_limits<std::uint64_t>::max()) return PhaseVmStatus::FrameLimit;
        for (auto i=base;i<top;++i) {
            const auto status=CheckValue(tags[i],stack[i]);
            if (status!=PhaseVmStatus::Ready) return status;
        }
        auto frame=MakeFrame(target,base,next_frame);
        frame->attachment=std::move(attachment);
        cmvm::RuntimeHooks hooks;hooks.resolved_script_call=&frame->function;
        const auto result=cmvm::step_instruction(context,hooks);
        ++observation.instructions;
        if (result!=cmvm::StepResult::Continue || context.function!=&frame->function ||
            context.frame_base!=stack.data()+base || context.stack_top!=stack.data()+frame->floor)
            return PhaseVmStatus::InterpreterMismatch;
        // Consumed arguments become the first locals; additional locals are unknown.
        std::fill(tags.begin()+top,tags.begin()+frame->floor,WordTag{});
        frames.push_back(std::move(frame));++next_frame;Observe();
        return PhaseVmStatus::Ready;
    }
    PhaseVmStatus Leave(std::uint8_t opcode, std::size_t top) {
        const auto& frame=*frames.back();
        const auto base=frame.base;
        const auto tag=opcode==kOpcodeReturnTop?tags[top-1]:WordTag{WordKind::Integer,0,{}};
        const auto value=opcode==kOpcodeReturnTop?stack[top-1]:cmvm::VmWord(opcode==kOpcodeReturnTrue?1:0);
        const auto status=CheckValue(tag,value);
        if (status!=PhaseVmStatus::Ready) return status;
        if (tag.kind==WordKind::LocalAddress && tag.frame==frame.identity) return PhaseVmStatus::DanglingLocal;
        const bool outer=frames.size()==1;
        const auto result=cmvm::step_instruction(context,{});
        ++observation.instructions;
        if (outer) {
            if (result!=cmvm::StepResult::Stop || context.stack_top!=stack.data()+base ||
                context.function || context.frame_base || context.instruction || context.result!=value)
                return PhaseVmStatus::InterpreterMismatch;
        } else {
            const auto& caller=*frames[frames.size()-2];
            if (result!=cmvm::StepResult::Continue || context.stack_top!=stack.data()+base+1 ||
                context.function!=&caller.function || context.frame_base!=stack.data()+caller.base || stack[base]!=value)
                return PhaseVmStatus::InterpreterMismatch;
        }
        std::fill(tags.begin()+base,tags.begin()+top,WordTag{});
        frames.pop_back();
        if(outer) {
            observation.return_value.reset();observation.return_archive_address.reset();
            observation.return_flag_bits_address.reset();
        }
        if (!outer) tags[base]=tag;
        else if (tag.kind==WordKind::Integer) observation.return_value=static_cast<std::int32_t>(value);
        else if (tag.kind==WordKind::ArchiveAddress) {
            const auto offset=static_cast<std::uintptr_t>(value)-reinterpret_cast<std::uintptr_t>(tag.archive->bytes().data());
            observation.return_archive_address=RetainedVmArchiveAddress{tag.archive,static_cast<std::uint32_t>(offset)};
        }
        else if(tag.kind==WordKind::FlagBitsAddress)observation.return_flag_bits_address=tag.flag_bits;
        Observe();
        return outer?PhaseVmStatus::Returned:PhaseVmStatus::Ready;
    }
    static PhaseVmStatus FlagFailure(runtime::native::EventFlagStatus status) noexcept {
        using S=runtime::native::EventFlagStatus;
        if(status==S::Retired || status==S::StalePlayerState || status==S::NullRuntime)return PhaseVmStatus::StaleNativeContext;
        if(status==S::InvalidName)return PhaseVmStatus::InvalidString;
        if(status==S::UnknownCommand)return PhaseVmStatus::NativeCallOwnerRequired;
        if(status==S::ForeignPlan || status==S::ConsumedPlan || status==S::UnknownOperation)return PhaseVmStatus::InterpreterMismatch;
        return PhaseVmStatus::NativeStateUnavailable;
    }
    PhaseVmStatus NativeFlag(std::size_t top) {
        auto& o=observation;
        if(o.call_arguments!=1)return PhaseVmStatus::InvalidNativeArguments;
        const auto base=top-1;
        if(tags[base].kind!=WordKind::ArchiveAddress)return PhaseVmStatus::InvalidNativeArguments;
        const auto checked=CheckValue(tags[base],stack[base]);
        if(checked!=PhaseVmStatus::Ready)return checked;
        const auto bytes=tags[base].archive->bytes();
        const auto offset=static_cast<std::uintptr_t>(stack[base])-reinterpret_cast<std::uintptr_t>(bytes.data());
        auto end=offset;
        while(end<bytes.size() && end-offset<=65535 && bytes[end]!=0)++end;
        if(end==bytes.size() || end-offset>65535)return PhaseVmStatus::InvalidString;
        const std::string_view name(reinterpret_cast<const char*>(bytes.data()+offset),end-offset);
        PreparedEventFlagCommand prepared;
        const auto status=flags->Prepare(o.registered_identifier,name,prepared);o.native_flag_status=status;
        if(status!=runtime::native::EventFlagStatus::Ok)return FlagFailure(status);
        struct Invocation {
            const NativeEventFlagCommands* owner{};
            PreparedEventFlagCommand* plan{};
            cmvm::VmWord argument{},value{};
            runtime::native::EventFlagResult result;
            runtime::native::EventFlagStatus status{runtime::native::EventFlagStatus::Ok};
            unsigned calls{};
            bool valid{true};
        } invocation;
        invocation.owner=flags.get();invocation.plan=&prepared;invocation.argument=stack[base];
        cmvm::CmFunctionView function;function.native=true;
        cmvm::RuntimeHooks hooks;hooks.resolved_native_call=&function;hooks.native_user=&invocation;
        hooks.invoke_native=[](void* user,cmvm::CmContextState&,const cmvm::VmWord* args,std::size_t count) -> cmvm::VmWord {
            auto& call=*static_cast<Invocation*>(user);
            if(++call.calls!=1 || count!=1 || !args || args[0]!=call.argument){call.valid=false;return 0;}
            call.status=call.owner->Commit(*call.plan,call.result);
            if(call.status!=runtime::native::EventFlagStatus::Ok)return 0;
            if(const auto integer=call.result.integer())call.value=*integer;
            else if(const auto address=call.result.address())call.value=static_cast<cmvm::VmWord>(*address);
            else call.valid=false;
            return call.value;
        };
        const auto before=context;
        const auto result=cmvm::step_instruction(context,hooks);
        o.native_flag_status=invocation.status;
        // A refused commit has made no bank write. Undo the interpreter's
        // already-consumed operand/result slot and PC, preserving the retry.
        if(invocation.status!=runtime::native::EventFlagStatus::Ok) {
            context=before;stack[base]=invocation.argument;return FlagFailure(invocation.status);
        }
        ++o.instructions;
        if(result!=cmvm::StepResult::Continue || !invocation.valid || invocation.calls!=1 ||
            context.stack_top!=stack.data()+base+1 || context.instruction!=before.instruction+4 || stack[base]!=invocation.value)
            return PhaseVmStatus::InterpreterMismatch;
        tags[base]={WordKind::Integer,0,{}};
        if(invocation.result.is_address()) {
            tags[base].kind=WordKind::FlagBitsAddress;tags[base].flag_bits=std::move(invocation.result);
        }
        ++o.native_flags;Observe();return PhaseVmStatus::Ready;
    }
    PhaseVmStatus NativeUnit(std::size_t top) {
        auto& o=observation;
        if(o.call_arguments!=1)return PhaseVmStatus::InvalidNativeArguments;
        const auto base=top-1;
        if(tags[base].kind!=WordKind::ArchiveAddress)return PhaseVmStatus::InvalidNativeArguments;
        const auto checked=CheckValue(tags[base],stack[base]);if(checked!=PhaseVmStatus::Ready)return checked;
        const auto bytes=tags[base].archive->bytes();
        const auto offset=static_cast<std::uintptr_t>(stack[base])-reinterpret_cast<std::uintptr_t>(bytes.data());
        auto end=offset;while(end<bytes.size() && bytes[end]!=0)++end;
        if(end==bytes.size())return PhaseVmStatus::InvalidString;
        o.native_unit_result=units->GetByPid(std::string_view(reinterpret_cast<const char*>(bytes.data()+offset),end-offset));
        const auto& query=*o.native_unit_result;
        if(query.status==UnitEventQueryStatus::Retired)return PhaseVmStatus::StaleNativeContext;
        if(query.status==UnitEventQueryStatus::InvalidName)return PhaseVmStatus::InvalidString;
        if(query.status!=UnitEventQueryStatus::Ok)return PhaseVmStatus::NativeStateUnavailable;
        struct Invocation {cmvm::VmWord argument{};std::int32_t value{};unsigned calls{};bool valid{true};};
        Invocation call{stack[base],query.value};
        cmvm::CmFunctionView function;function.native=true;
        cmvm::RuntimeHooks hooks;hooks.resolved_native_call=&function;hooks.native_user=&call;
        hooks.invoke_native=[](void* user,cmvm::CmContextState&,const cmvm::VmWord* args,std::size_t count)->cmvm::VmWord {
            auto& invocation=*static_cast<Invocation*>(user);
            if(++invocation.calls!=1 || count!=1 || !args || args[0]!=invocation.argument)invocation.valid=false;
            return invocation.value;
        };
        const auto pc=context.instruction;const auto result=cmvm::step_instruction(context,hooks);++o.instructions;
        if(result!=cmvm::StepResult::Continue || !call.valid || call.calls!=1 ||
            context.instruction!=pc+4 || context.stack_top!=stack.data()+base+1 || stack[base]!=call.value)
            return PhaseVmStatus::InterpreterMismatch;
        tags[base]={WordKind::Integer,0,{}};++o.native_units;Observe();return PhaseVmStatus::Ready;
    }
    PhaseVmStatus NativeCamera(std::size_t top,const ProcEventVmAccess* access) {
        auto& o=observation;
        if(o.call_arguments!=0)return PhaseVmStatus::InvalidNativeArguments;
        if(top==stack.size())return PhaseVmStatus::StackLimit;
        if(!access)return PhaseVmStatus::NativeStateUnavailable;
        struct Invocation {
            map::native::NativeCameraWait* camera{};const ProcEventVmAccess* access{};
            map::native::CameraWaitStatus status{map::native::CameraWaitStatus::UnknownState};
            map::native::CameraWaitOutcome outcome;unsigned calls{};
        } invocation{camera.get(),access};
        cmvm::CmFunctionView function;function.native=true;
        cmvm::RuntimeHooks hooks;hooks.resolved_native_call=&function;hooks.native_user=&invocation;
        hooks.invoke_native=[](void* user,cmvm::CmContextState& ctx,const cmvm::VmWord*,std::size_t count)->cmvm::VmWord {
            auto& call=*static_cast<Invocation*>(user);
            if(++call.calls!=1 || count)return 0;
            call.status=call.camera->EventWait(call.access->process_access(),call.access->current_event(),call.outcome);
            if(call.status==map::native::CameraWaitStatus::Ready)ctx.yielded=call.outcome.yielded;
            return call.outcome.value.value_or(0);
        };
        const auto before=context;const auto word=stack[top];
        const auto result=cmvm::step_instruction(context,hooks);o.native_camera_status=invocation.status;
        if(invocation.status!=map::native::CameraWaitStatus::Ready) {
            context=before;stack[top]=word;
            return invocation.status==map::native::CameraWaitStatus::Retired?PhaseVmStatus::StaleNativeContext:PhaseVmStatus::NativeStateUnavailable;
        }
        ++o.instructions;
        if(result!=cmvm::StepResult::Continue || invocation.calls!=1 || context.instruction!=before.instruction+4 ||
            context.stack_top!=stack.data()+top+1 || stack[top]!=invocation.outcome.value.value_or(0) || context.yielded!=invocation.outcome.yielded)
            return PhaseVmStatus::InterpreterMismatch;
        tags[top]=invocation.outcome.value?WordTag{WordKind::Integer,0,{}}:WordTag{};
        ++o.native_cameras;Observe();return context.yielded?PhaseVmStatus::Yielded:PhaseVmStatus::Ready;
    }
    PhaseVmStatus NativeCameraCommand(std::size_t top,const ProcEventVmAccess* access,bool near_distance) {
        auto& o=observation;
        const std::size_t argc=near_distance?0:1;
        if(o.call_arguments!=argc || top<argc || (argc && tags[top-1].kind!=WordKind::Integer))return PhaseVmStatus::InvalidNativeArguments;
        if(top-argc==stack.size())return PhaseVmStatus::StackLimit;
        if(!access)return PhaseVmStatus::NativeStateUnavailable;
        const auto base=top-argc;
        struct Invocation {
            NativeEventCamera* owner{};const ProcEventVmAccess* access{};cmvm::VmWord argument{};
            bool near_distance{};
            EventCameraStatus status{EventCameraStatus::UnknownChapter};EventCameraOutcome outcome;unsigned calls{};
        } invocation{camera_commands.get(),access,argc?stack[base]:0,near_distance};
        cmvm::CmFunctionView function;function.native=true;
        cmvm::RuntimeHooks hooks;hooks.resolved_native_call=&function;hooks.native_user=&invocation;
        hooks.invoke_native=[](void* user,cmvm::CmContextState&,const cmvm::VmWord* args,std::size_t count)->cmvm::VmWord {
            auto& call=*static_cast<Invocation*>(user);
            if(++call.calls!=1 || count!=(call.near_distance?0u:1u) || (!call.near_distance && (!args || args[0]!=call.argument)))return 0;
            call.status=call.near_distance?call.owner->SetDistanceFromNear(call.access->process_access(),call.outcome):
                call.owner->SetAngle(call.access->process_access(),call.access->current_event(),static_cast<std::int32_t>(call.argument),call.outcome);
            return call.outcome.value.value_or(0);
        };
        const auto before=context;const auto word=stack[base];
        const auto result=cmvm::step_instruction(context,hooks);o.native_camera_command_status=invocation.status;
        if(invocation.status!=EventCameraStatus::Ready) {
            context=before;stack[base]=word;
            return invocation.status==EventCameraStatus::Retired?PhaseVmStatus::StaleNativeContext:PhaseVmStatus::NativeStateUnavailable;
        }
        ++o.instructions;
        if(result!=cmvm::StepResult::Continue || invocation.calls!=1 || context.instruction!=before.instruction+4 ||
            context.stack_top!=stack.data()+base+1 || stack[base]!=invocation.outcome.value.value_or(0) || context.yielded!=before.yielded)
            return PhaseVmStatus::InterpreterMismatch;
        tags[base]=invocation.outcome.value?WordTag{WordKind::Integer,0,{}}:WordTag{};
        ++o.native_camera_commands;Observe();return PhaseVmStatus::Ready;
    }
    PhaseVmStatus NativeQuery(std::size_t top,const ProcEventVmAccess* access) {
        auto& o=observation;
        if(camera && o.registered_identifier=="ev::CameraWait")return NativeCamera(top,access);
        if(camera_commands && o.registered_identifier=="ev::CameraSetAngle")return NativeCameraCommand(top,access,false);
        if(camera_commands && o.registered_identifier=="ev::CameraSetDistanceFromNear")return NativeCameraCommand(top,access,true);
        if(flags && NativeEventFlagCommands::Recognizes(o.registered_identifier))return NativeFlag(top);
        if(units && o.registered_identifier=="ev::UnitGetByPID")return NativeUnit(top);
        if(!queries)return PhaseVmStatus::NativeCallOwnerRequired;
        const auto spec=NativePhaseEventQueries::Find(o.registered_identifier);
        if(!spec) {o.native_query_status=PhaseQueryStatus::Unimplemented;return PhaseVmStatus::NativeCallOwnerRequired;}
        if(o.call_arguments!=spec->arguments) {
            o.native_query_status=PhaseQueryStatus::ArgumentCount;return PhaseVmStatus::InvalidNativeArguments;
        }
        const auto base=top-o.call_arguments;
        if(base==stack.size())return PhaseVmStatus::StackLimit;
        std::vector<std::int32_t> args;
        for(auto i=base;i<top;++i) {
            if(tags[i].kind!=WordKind::Integer)return PhaseVmStatus::InvalidNativeArguments;
            args.push_back(static_cast<std::int32_t>(stack[i]));
        }
        struct Invocation {std::int32_t value{};std::size_t count{};unsigned calls{};};
        Invocation invocation;
        const auto status=queries->Query(spec->query,args,invocation.value);o.native_query_status=status;
        if(status!=PhaseQueryStatus::Ok) {
            if(status==PhaseQueryStatus::StalePhase || status==PhaseQueryStatus::Retired)return PhaseVmStatus::StaleNativeContext;
            return PhaseVmStatus::NativeStateUnavailable;
        }
        // These concrete services are reads, so preflight can obtain the value
        // without a gameplay commit. The source interpreter still consumes the
        // actual call operands and publishes the callback result exactly once.
        cmvm::CmFunctionView function;function.native=true;
        cmvm::RuntimeHooks hooks;hooks.resolved_native_call=&function;hooks.native_user=&invocation;
        hooks.invoke_native=[](void* user,cmvm::CmContextState&,const cmvm::VmWord*,std::size_t count) -> cmvm::VmWord {
            auto& call=*static_cast<Invocation*>(user);++call.calls;call.count=count;return call.value;
        };
        const auto pc=context.instruction;
        const auto result=cmvm::step_instruction(context,hooks);++o.instructions;
        if(result!=cmvm::StepResult::Continue || invocation.calls!=1 || invocation.count!=o.call_arguments ||
            context.stack_top!=stack.data()+base+1 || context.instruction!=pc+4 || stack[base]!=invocation.value)
            return PhaseVmStatus::InterpreterMismatch;
        std::fill(tags.begin()+base,tags.begin()+top,WordTag{});tags[base]={WordKind::Integer,0,{}};
        ++o.native_queries;Observe();return PhaseVmStatus::Ready;
    }
    PhaseVmStatus Step(const ProcEventVmAccess* access) {
        auto& o=observation;
        const auto& frame=*frames.back();
        const auto owner=frame.reference.table()->archive();
        const auto bytes=owner->bytes();
        const auto pc=std::size_t(o.code_offset);
        const auto top=static_cast<std::size_t>(context.stack_top-stack.data());
        const auto depth=top-frame.floor;
        if (!Fits(pc,1,bytes.size())) return PhaseVmStatus::CodeBounds;
        const auto op=bytes[pc]; o.opcode=op;
        std::size_t length=1, pop=0, push=0;
        WordTag pushed{WordKind::Integer,0,{}};
        std::optional<std::size_t> assigned_local;
        bool numeric=false, truth=false;
        switch (op) {
        case kOpcodeLocalLoadI8: case kOpcodeLocalAddressI8: length=2;push=1;break;
        case kOpcodePushI8: length=2;push=1;break;
        case kOpcodePushI16Be: length=3;push=1;break;
        case kOpcodePushWord32Be: case kOpcodePushWord32BeAlt: length=5;push=1;break;
        case kOpcodePushArchiveAddressI8: length=2;push=1;pushed={WordKind::ArchiveAddress,0,owner};break;
        case kOpcodePushArchiveAddressI16Be: length=3;push=1;pushed={WordKind::ArchiveAddress,0,owner};break;
        case kOpcodeLoadIndirectKeepAddress: push=1;break;
        case kOpcodeDuplicateTopWord: push=1;break;
        case kOpcodeDrop: pop=1;break;
        case kOpcodeStoreIndirect: pop=2;break;
        case kOpcodeSubtractInt: case kOpcodeCompareLessEqualInt: case kOpcodeCompareGreaterEqualInt:
        case kOpcodeDivideInt: case kOpcodeRemainderInt: case kOpcodeCompareGreaterInt:
            pop=2;push=1;numeric=true;break;
        case kOpcodeCompareEqualWord: pop=2;push=1;break;
        case kOpcodeLogicalNot: pop=1;push=1;truth=true;break;
        case kOpcodeReturnTop: pop=1;break;
        case kOpcodeBranchI16Be: length=3;break;
        case kOpcodeBranchIfZeroI16Be: length=3;pop=1;truth=true;break;
        case kOpcodeYield: case kOpcodeReturnFalse: case kOpcodeReturnTrue: break;
        case kOpcodeInlineWordSkip: length=5;break;
        case kOpcodeCallIdent: length=4;break;
        case kOpcodeCallFunctionIndex:
            if (!Fits(pc,2,bytes.size())) return PhaseVmStatus::CodeBounds;
            length=(bytes[pc+1]&0x80u)?3:2;break;
        default: return PhaseVmStatus::UnsupportedOpcode;
        }
        if (!Fits(pc,length,bytes.size())) return PhaseVmStatus::CodeBounds;
        if (depth<pop) return PhaseVmStatus::StackUnderflow;
        if ((op==kOpcodeLoadIndirectKeepAddress || op==kOpcodeDuplicateTopWord) && depth==0)
            return PhaseVmStatus::StackUnderflow;
        if (push>stack.size()-(top-pop)) return PhaseVmStatus::StackLimit;
        if (numeric) for (std::size_t i=0;i<pop;++i)
            if (tags[top-1-i].kind!=WordKind::Integer) return PhaseVmStatus::InvalidValue;
        if ((op==kOpcodeDivideInt || op==kOpcodeRemainderInt) && stack[top-1]==0)
            return PhaseVmStatus::DivisionByZero;
        if (op==kOpcodeDuplicateTopWord) {
            const auto status=CheckValue(tags[top-1],stack[top-1]);
            if(status!=PhaseVmStatus::Ready)return status;
            pushed=tags[top-1];
        }
        if (op==kOpcodeCompareEqualWord) {
            for(std::size_t i=0;i<2;++i) {
                const auto status=CheckValue(tags[top-1-i],stack[top-1-i]);
                if(status!=PhaseVmStatus::Ready)return status;
            }
            // Retail pointer equality is preserved by full host addresses.
            // Only the null integer is comparable with an address: accepting
            // any other integer would invent an emulated-to-host pointer map.
            const bool left_integer=tags[top-2].kind==WordKind::Integer;
            const bool right_integer=tags[top-1].kind==WordKind::Integer;
            if(left_integer!=right_integer && stack[top-(left_integer?2:1)]!=0)
                return PhaseVmStatus::InvalidValue;
        }
        if (truth) {
            const auto status=CheckValue(tags[top-1],stack[top-1]);
            if (status!=PhaseVmStatus::Ready) return status;
        }
        if (op==kOpcodeLocalLoadI8 || op==kOpcodeLocalAddressI8) {
            const auto index=std::bit_cast<std::int8_t>(bytes[pc+1]);
            if (index<0 || index>=frame.function.local_words) return PhaseVmStatus::InvalidLocal;
            if (op==kOpcodeLocalAddressI8) pushed={WordKind::LocalAddress,frame.identity,{}};
            else {
                const auto cell=frame.base+static_cast<std::size_t>(index);
                pushed=tags[cell];
                const auto status=CheckValue(pushed,stack[cell]);
                if (status!=PhaseVmStatus::Ready) return status;
            }
        }
        if (op==kOpcodeLoadIndirectKeepAddress) {
            std::size_t index{};
            auto status=LocalCell(tags[top-1],stack[top-1],index);
            if (status!=PhaseVmStatus::Ready) return status;
            pushed=tags[index];status=CheckValue(pushed,stack[index]);
            if (status!=PhaseVmStatus::Ready) return status;
        }
        if (op==kOpcodeStoreIndirect) {
            // Only tagged locals of an active retained frame may be dereferenced.
            std::size_t index{};
            auto status=LocalCell(tags[top-2],stack[top-2],index);
            if (status!=PhaseVmStatus::Ready) return status;
            status=CheckValue(tags[top-1],stack[top-1]);
            if (status!=PhaseVmStatus::Ready) return status;
            assigned_local=index;
        }
        if (op==kOpcodePushArchiveAddressI8 || op==kOpcodePushArchiveAddressI16Be || op==kOpcodeCallIdent) {
            const auto offset=op==kOpcodePushArchiveAddressI8 ?
                std::int64_t(std::bit_cast<std::int8_t>(bytes[pc+1])) : std::int64_t(I16(bytes,pc+1));
            const auto address=std::int64_t(U32(bytes,0x20))+offset;
            if (address<0 || address>=static_cast<std::int64_t>(bytes.size())) return PhaseVmStatus::InvalidString;
            if (op==kOpcodeCallIdent) {
                auto end=static_cast<std::size_t>(address);
                while (end<bytes.size() && bytes[end]!=0) ++end;
                if (end==bytes.size()) return PhaseVmStatus::InvalidString;
                o.identifier.assign(reinterpret_cast<const char*>(bytes.data()+address),
                    end-static_cast<std::size_t>(address));
                o.call_arguments=bytes[pc+3];
                o.registered_identifier.clear();
                if(session) {
                    const auto found=session->Find(o.identifier);o.registered_identifier=found.registered_identifier;
                    using L=cmvm::native::LiveScriptLookupStatus;
                    if(found.status==L::FoundScript)return Enter(found.function,top,depth,found.attachment);
                    if(found.status==L::InvalidIdentifier)return PhaseVmStatus::InvalidString;
                    if(found.status==L::Retired || found.status==L::DetachedFunction)return PhaseVmStatus::StaleScriptSession;
                    if(depth<o.call_arguments)return PhaseVmStatus::StackUnderflow;
                    if(found.status==L::NativeOwnerRequired)return NativeQuery(top,access);
                    return PhaseVmStatus::IdentifierCallOwnerRequired;
                }
                if (registry) {
                    const auto found=registry->Find(o.identifier);
                    o.registered_identifier=found.registered_identifier;
                    if (found.status==cmvm::native::ScriptLookupStatus::FoundScript)
                        return Enter(found.function,top,depth);
                    if (found.status==cmvm::native::ScriptLookupStatus::InvalidIdentifier) return PhaseVmStatus::InvalidString;
                    if (depth<o.call_arguments) return PhaseVmStatus::StackUnderflow;
                    if (found.status==cmvm::native::ScriptLookupStatus::NativeOwnerRequired) return NativeQuery(top,access);
                }
                if (depth<o.call_arguments) return PhaseVmStatus::StackUnderflow;
                return PhaseVmStatus::IdentifierCallOwnerRequired;
            }
        }
        if (op==kOpcodeCallFunctionIndex) {
            o.call_index=static_cast<std::uint16_t>(length==2 ? bytes[pc+1] :
                unsigned(bytes[pc+1]&0x7fu)*128u+bytes[pc+2]);
            cmvm::native::ScriptFunctionRef target;
            if (frame.reference.table()->Get(o.call_index,target)!=cmvm::native::ScriptFunctionStatus::Ok)
                return PhaseVmStatus::InvalidCall;
            return Enter(target,top,depth,frame.attachment);
        }
        if (op==kOpcodeBranchI16Be || op==kOpcodeBranchIfZeroI16Be) {
            if (op==kOpcodeBranchI16Be || stack[top-1]==0) {
                const auto target=std::int64_t(pc)+1+I16(bytes,pc+1);
                if (target<0x28 || target>=static_cast<std::int64_t>(bytes.size())) return PhaseVmStatus::CodeBounds;
            }
        }
        if (op==kOpcodeReturnTop || op==kOpcodeReturnFalse || op==kOpcodeReturnTrue) return Leave(op,top);
        const auto result=cmvm::step_instruction(context,{});
        ++o.instructions;
        Observe();
        if (result==cmvm::StepResult::Yield && op==kOpcodeYield &&
            context.stack_top==stack.data()+top && context.yielded && !context.stopped)
            return PhaseVmStatus::Yielded;
        if (result!=cmvm::StepResult::Continue ||
            context.stack_top!=stack.data()+top-pop+push) return PhaseVmStatus::InterpreterMismatch;
        if (assigned_local) tags[*assigned_local]=tags[top-1];
        std::fill(tags.begin()+top-pop,tags.begin()+top,WordTag{});
        for (std::size_t i=0;i<push;++i) tags[top-pop+i]=pushed;
        return PhaseVmStatus::Ready;
    }
};
PhaseEventVm::PhaseEventVm(std::unique_ptr<State> state):state_(std::move(state)) {}
PhaseEventVm::~PhaseEventVm()=default;
PhaseVmStatus PhaseEventVm::Create(const PhaseEventSelection& selected, std::size_t words,
    std::unique_ptr<PhaseEventVm>& out) {
    return Create(selected,words,{},out);
}
PhaseVmStatus PhaseEventVm::Create(const PhaseEventSelection& selected, std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptFunctionRegistry> registry, std::unique_ptr<PhaseEventVm>& out) {
    return Create(selected,words,std::move(registry),{},out);
}
PhaseVmStatus PhaseEventVm::Create(const PhaseEventSelection& selected, std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptFunctionRegistry> registry,
    std::shared_ptr<const NativePhaseEventQueries> queries, std::unique_ptr<PhaseEventVm>& out) {
    return Create(selected,words,std::move(registry),std::move(queries),{},out);
}
PhaseVmStatus PhaseEventVm::Create(const PhaseEventSelection& selected, std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptFunctionRegistry> registry,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::shared_ptr<const NativeEventFlagCommands> flags,
    std::unique_ptr<PhaseEventVm>& out) {
    return CreateWithServices(selected,words,std::move(registry),{std::move(queries),std::move(flags),{}},out);
}
PhaseVmStatus PhaseEventVm::CreateWithServices(const PhaseEventSelection& selected,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptFunctionRegistry> registry,NativeEventServices services,
    std::unique_ptr<PhaseEventVm>& out) {
    const auto* declaration=selected.declaration();
    if (!declaration) return PhaseVmStatus::InvalidSelection;
    if (words>65536 || words<std::size_t(declaration->local_words)+3) return PhaseVmStatus::StackLimit;
    if(services.phase && services.phase->Validate()!=PhaseQueryStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    auto state=std::make_unique<State>();
    state->selected=selected;state->registry=std::move(registry);state->queries=std::move(services.phase);
    state->flags=std::move(services.flags);state->units=std::move(services.units);state->camera=std::move(services.camera);state->camera_commands=std::move(services.camera_commands);
    std::shared_ptr<const cmvm::native::ScriptFunctionTable> table;
    if (cmvm::native::ScriptFunctionTable::Read(selected.archive(),table)!=cmvm::native::ScriptFunctionStatus::Ok)
        return PhaseVmStatus::InvalidFunction;
    cmvm::native::ScriptFunctionRef reference;
    if (table->Get(declaration->function_index,reference)!=cmvm::native::ScriptFunctionStatus::Ok)
        return PhaseVmStatus::InvalidFunction;
    return CreateRoot(reference,{},words,std::move(state),out);
}
PhaseVmStatus PhaseEventVm::CreateRoot(const cmvm::native::ScriptFunctionRef& reference,
    std::span<const std::int32_t> arguments,std::size_t words,std::unique_ptr<State> state,
    std::unique_ptr<PhaseEventVm>& out) {
    const auto* info=reference.info();
    if(!info)return PhaseVmStatus::InvalidFunction;
    const auto argc=info->type==0?std::size_t(info->argument_count):0;
    if(argc>=128 || argc>info->local_words || arguments.size()!=argc)return PhaseVmStatus::InvalidCall;
    if(words>65536 || words<std::size_t(info->local_words)+3)return PhaseVmStatus::StackLimit;
    if(state->queries && state->queries->Validate()!=PhaseQueryStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    if(state->flags && state->flags->Validate()!=runtime::native::EventFlagStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    if(state->units && state->units->Validate()!=UnitEventQueryStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    if(!NativeEventServices{state->queries,state->flags,state->units,state->camera,state->camera_commands}.UsesOneRuntime())
        return PhaseVmStatus::MismatchedNativeContext;
    state->root=reference;state->stack.resize(words);state->tags.resize(words);
    for(std::size_t i=0;i<argc;++i) {
        state->stack[i]=arguments[i];state->tags[i]={WordKind::Integer,0,{}};
    }
    state->frames.push_back(State::MakeFrame(reference,0,state->next_frame++));
    state->context.stack_base=state->stack.data();state->context.stack_top=state->stack.data()+argc;
    if (!cmvm::set_function(state->context,state->frames.back()->function)) return PhaseVmStatus::InterpreterMismatch;
    state->Observe();
    out=std::unique_ptr<PhaseEventVm>(new PhaseEventVm(std::move(state)));
    return PhaseVmStatus::Ready;
}
PhaseVmStatus PhaseEventVm::CreateAttached(const cmvm::native::AttachedPhaseSelection& selected,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::unique_ptr<PhaseEventVm>& out) {
    return CreateAttached(selected,words,std::move(session),std::move(queries),{},out);
}
PhaseVmStatus PhaseEventVm::CreateAttached(const cmvm::native::AttachedPhaseSelection& selected,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::shared_ptr<const NativeEventFlagCommands> flags,
    std::unique_ptr<PhaseEventVm>& out) {
    return CreateAttachedWithServices(selected,words,std::move(session),{std::move(queries),std::move(flags),{}},out);
}
PhaseVmStatus PhaseEventVm::CreateAttachedWithServices(const cmvm::native::AttachedPhaseSelection& selected,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,NativeEventServices services,
    std::unique_ptr<PhaseEventVm>& out) {
    if(!session || !selected || !session->IsAttached(selected.attachment()) ||
        selected.attachment().table()->archive()!=selected.selection().archive())return PhaseVmStatus::StaleScriptSession;
    cmvm::native::AttachedScriptFunction function;
    if(session->SelectFunction(selected.attachment(),selected.selection().declaration()->function_index,function)
        !=cmvm::native::ScriptSessionStatus::Found)return PhaseVmStatus::InvalidFunction;
    std::unique_ptr<PhaseEventVm> next;
    const auto status=CreateAttachedFunctionWithServices(function,{},words,std::move(session),std::move(services),next);
    if(status!=PhaseVmStatus::Ready)return status;
    next->state_->selected=selected.selection();
    out=std::move(next);return PhaseVmStatus::Ready;
}
PhaseVmStatus PhaseEventVm::CreateAttachedFunction(const cmvm::native::AttachedScriptFunction& selected,
    std::span<const std::int32_t> arguments,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::unique_ptr<PhaseEventVm>& out) {
    return CreateAttachedFunction(selected,arguments,words,std::move(session),std::move(queries),{},out);
}
PhaseVmStatus PhaseEventVm::CreateAttachedFunction(const cmvm::native::AttachedScriptFunction& selected,
    std::span<const std::int32_t> arguments,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,
    std::shared_ptr<const NativePhaseEventQueries> queries,std::shared_ptr<const NativeEventFlagCommands> flags,
    std::unique_ptr<PhaseEventVm>& out) {
    return CreateAttachedFunctionWithServices(selected,arguments,words,std::move(session),{std::move(queries),std::move(flags),{}},out);
}
PhaseVmStatus PhaseEventVm::CreateAttachedFunctionWithServices(const cmvm::native::AttachedScriptFunction& selected,
    std::span<const std::int32_t> arguments,std::size_t words,
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session,NativeEventServices services,
    std::unique_ptr<PhaseEventVm>& out) {
    if(!session || !selected || !session->IsAttached(selected.attachment()) ||
        selected.attachment().table()!=selected.function().table())return PhaseVmStatus::StaleScriptSession;
    auto state=std::make_unique<State>();state->session=std::move(session);state->queries=std::move(services.phase);
    state->flags=std::move(services.flags);state->units=std::move(services.units);state->camera=std::move(services.camera);state->camera_commands=std::move(services.camera_commands);
    std::unique_ptr<PhaseEventVm> next;
    const auto status=CreateRoot(selected.function(),arguments,words,std::move(state),next);
    if(status!=PhaseVmStatus::Ready)return status;
    next->state_->frames.front()->attachment=selected.attachment();
    next->state_->root_attachment=selected.attachment();
    out=std::move(next);return PhaseVmStatus::Ready;
}
PhaseVmStatus PhaseEventVm::RetargetAttachedFunction(const cmvm::native::AttachedScriptFunction& selected,
    std::span<const std::int32_t> arguments) {
    auto& s=*state_;
    if(s.observation.status!=PhaseVmStatus::Returned || !s.frames.empty())return PhaseVmStatus::ContextNotReturned;
    if(!s.session || !s.SessionValid() || !selected || !s.session->IsAttached(selected.attachment()) ||
        selected.attachment().table()!=selected.function().table())return PhaseVmStatus::StaleScriptSession;
    if(s.queries && s.queries->Validate()!=PhaseQueryStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    if(s.flags && s.flags->Validate()!=runtime::native::EventFlagStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    if(s.units && s.units->Validate()!=UnitEventQueryStatus::Ok)return PhaseVmStatus::StaleNativeContext;
    const auto& info=*selected.function().info();
    const auto argc=info.type==0?std::size_t(info.argument_count):0;
    if(argc>=128 || argc>info.local_words || arguments.size()!=argc)return PhaseVmStatus::InvalidCall;
    if(std::size_t(info.local_words)+3>s.stack.size())return PhaseVmStatus::StackLimit;
    if(s.next_frame==std::numeric_limits<std::uint64_t>::max())return PhaseVmStatus::FrameLimit;
    if(s.context.stack_top!=s.stack.data() || s.context.function || s.context.frame_base || s.context.instruction)
        return PhaseVmStatus::InterpreterMismatch;
    auto frame=State::MakeFrame(selected.function(),0,s.next_frame);
    frame->attachment=selected.attachment();
    // Do not reconstruct context or resize storage: SetFunction leaves result
    // and unused raw words alone. Only explicitly supplied arguments are known.
    std::fill(s.tags.begin(),s.tags.end(),WordTag{});
    for(std::size_t i=0;i<argc;++i) {s.stack[i]=arguments[i];s.tags[i]={WordKind::Integer,0,{}};}
    s.context.stack_top=s.stack.data()+argc;
    if(!cmvm::set_function(s.context,frame->function))return PhaseVmStatus::InterpreterMismatch;
    s.frames.push_back(std::move(frame));++s.next_frame;
    s.root=selected.function();s.root_attachment=selected.attachment();s.selected={};
    auto& o=s.observation;o.status=PhaseVmStatus::Ready;o.opcode.reset();o.identifier.clear();
    o.call_index=0;o.call_arguments=0;o.registered_identifier.clear();o.native_query_status.reset();o.native_flag_status.reset();o.native_unit_result.reset();o.native_camera_status.reset();o.native_camera_command_status.reset();
    s.Observe();return PhaseVmStatus::Ready;
}
PhaseVmObservation PhaseEventVm::Run(std::size_t budget) {return RunImpl(budget,nullptr);}
PhaseVmObservation PhaseEventVm::Run(std::size_t budget,const ProcEventVmAccess& access) {return RunImpl(budget,&access);}
PhaseVmObservation PhaseEventVm::RunImpl(std::size_t budget,const ProcEventVmAccess* access) {
    auto& o=state_->observation;
    if (o.status!=PhaseVmStatus::Ready && o.status!=PhaseVmStatus::InstructionBudget &&
        o.status!=PhaseVmStatus::Yielded && o.status!=PhaseVmStatus::NativeStateUnavailable &&
        !(state_->session && (o.status==PhaseVmStatus::IdentifierCallOwnerRequired ||
            o.status==PhaseVmStatus::NativeCallOwnerRequired))) return o;
    if(!state_->SessionValid()){o.status=PhaseVmStatus::StaleScriptSession;return o;}
    if(state_->units && state_->units->Validate()!=UnitEventQueryStatus::Ok){o.status=PhaseVmStatus::StaleNativeContext;return o;}
    if(state_->flags) {
        const auto valid=state_->flags->Validate();
        if(valid!=runtime::native::EventFlagStatus::Ok) {o.native_flag_status=valid;o.status=PhaseVmStatus::StaleNativeContext;return o;}
    }
    if(state_->queries) {
        const auto valid=state_->queries->Validate();
        if(valid!=PhaseQueryStatus::Ok) {o.native_query_status=valid;o.status=PhaseVmStatus::StaleNativeContext;return o;}
    }
    // This is a host work quantum, not emulated event timing. It cannot overflow
    // the monotonic executed-instruction count or block the host indefinitely.
    if (budget>1000000) budget=1000000;
    for (std::size_t i=0;i<budget;++i) {
        if(!state_->SessionValid()){o.status=PhaseVmStatus::StaleScriptSession;return o;}
        if (o.instructions==std::numeric_limits<std::uint64_t>::max()) break;
        o.status=state_->Step(access);
        if (o.status!=PhaseVmStatus::Ready) return o;
    }
    o.status=PhaseVmStatus::InstructionBudget;
    return o;
}
const PhaseVmObservation& PhaseEventVm::observation() const noexcept { return state_->observation; }
const cmvm::native::ScriptFunctionRef& PhaseEventVm::root_function() const noexcept {return state_->root;}
const PhaseEventSelection& PhaseEventVm::selection() const noexcept { return state_->selected; }
}
