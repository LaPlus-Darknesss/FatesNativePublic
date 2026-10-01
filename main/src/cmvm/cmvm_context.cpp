#include "fates/cmvm/cmvm.hpp"
#include "fates/detail/cmvm_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <bit>

namespace fates::cmvm {
namespace {
using fates::detail::cmvm::kOpcodeBranchI16Be;
using fates::detail::cmvm::kOpcodeCallFunctionIndex;
using fates::detail::cmvm::kOpcodeCompareGreaterEqualInt;
using fates::detail::cmvm::kOpcodeCompareLessEqualInt;
using fates::detail::cmvm::kOpcodeLoadIndirectKeepAddress;
using fates::detail::cmvm::kOpcodeLogicalNot;
using fates::detail::cmvm::kOpcodeReturnTop;
using fates::detail::cmvm::kOpcodeSubtractInt;
using fates::detail::cmvm::kOpcodeBranchIfZeroI16Be;
using fates::detail::cmvm::kOpcodeCallIdent;
using fates::detail::cmvm::kOpcodeDrop;
using fates::detail::cmvm::kOpcodeInlineWordSkip;
using fates::detail::cmvm::kOpcodeLocalAddressI8;
using fates::detail::cmvm::kOpcodeLocalLoadI8;
using fates::detail::cmvm::kOpcodePushArchiveAddressI16Be;
using fates::detail::cmvm::kOpcodePushArchiveAddressI8;
using fates::detail::cmvm::kOpcodePushI16Be;
using fates::detail::cmvm::kOpcodePushI8;
using fates::detail::cmvm::kOpcodePushWord32Be;
using fates::detail::cmvm::kOpcodePushWord32BeAlt;
using fates::detail::cmvm::kOpcodeReturnFalse;
using fates::detail::cmvm::kOpcodeReturnTrue;
using fates::detail::cmvm::kOpcodeYield;
using fates::detail::cmvm::kOpcodeStoreIndirect;
using fates::detail::cmvm::kOpcodeDivideInt;
using fates::detail::cmvm::kOpcodeRemainderInt;
using fates::detail::cmvm::kOpcodeCompareEqualWord;
using fates::detail::cmvm::kOpcodeCompareGreaterInt;
using fates::detail::cmvm::kOpcodeDuplicateTopWord;

std::int8_t ReadI8(const std::byte*& p) noexcept {
    const auto value = static_cast<std::int8_t>(std::to_integer<std::uint8_t>(*p));
    ++p;
    return value;
}

std::int16_t ReadI16Be(const std::byte*& p) noexcept {
    const auto hi = std::to_integer<std::uint8_t>(p[0]);
    const auto lo = std::to_integer<std::uint8_t>(p[1]);
    p += 2;
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(hi) << 8U) | lo);
}

std::int32_t ReadWord32Be(const std::byte*& p) noexcept {
    const auto value = (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) << 24U) |
                       (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1])) << 16U) |
                       (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2])) << 8U) |
                       static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[3]));
    p += 4;
    return static_cast<std::int32_t>(value);
}

std::size_t ReadFunctionIndex(const std::byte*& p) noexcept {
    const auto first = std::to_integer<std::uint8_t>(*p++);
    if ((first & 0x80U) == 0) return first;
    const auto second = std::to_integer<std::uint8_t>(*p++);
    return static_cast<std::size_t>((first & 0x7fU) * 0x80U + second);
}

std::int32_t WordAsI32(const VmWord value) noexcept {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uintptr_t>(value)));
}

VmWord SubI32(const VmWord lhs, const VmWord rhs) noexcept {
    const auto result = static_cast<std::uint32_t>(WordAsI32(lhs)) - static_cast<std::uint32_t>(WordAsI32(rhs));
    return static_cast<VmWord>(static_cast<std::int32_t>(result));
}

VmWord Pop(CmContextState& ctx) noexcept {
    --ctx.stack_top;
    return *ctx.stack_top;
}

void Push(CmContextState& ctx, const VmWord value) noexcept {
    *ctx.stack_top = value;
    ++ctx.stack_top;
}

const std::byte* StringBase(const CmContextState& ctx) noexcept {
    return (ctx.function && ctx.function->archive) ? ctx.function->archive->string_base : nullptr;
}

StepResult RuntimeBoundary(CmContextState& ctx, const RuntimeHooks& hooks, const std::uint8_t opcode) {
    (void)ctx;
    if (hooks.on_runtime_boundary) hooks.on_runtime_boundary(opcode);
    return StepResult::RuntimeBoundary;
}

StepResult ReturnConstant(CmContextState& ctx, const VmWord value, const RuntimeHooks& hooks, const std::uint8_t opcode) {
    if (!ctx.function || !ctx.frame_base) return RuntimeBoundary(ctx, hooks, opcode);
    const auto local_words = static_cast<std::ptrdiff_t>(ctx.function->local_words);
    VmWord* const current_frame = ctx.frame_base;
    VmWord* const saved = current_frame + local_words;

    const auto* const old_instruction = reinterpret_cast<const std::byte*>(saved[0]);
    const auto* const old_function = reinterpret_cast<const CmFunctionView*>(saved[1]);
    auto* const old_frame = reinterpret_cast<VmWord*>(saved[2]);

    ctx.frame_base = old_frame;
    ctx.function = old_function;
    ctx.instruction = old_instruction;

    if (!old_instruction) {
        ctx.stack_top = current_frame;
        ctx.result = value;
        ctx.stopped = true;
        return StepResult::Stop;
    }

    ctx.stack_top = current_frame;
    Push(ctx, value);
    return StepResult::Continue;
}
} // namespace

VmWord call_native(CmContextState& ctx, const CmFunctionView& fn, const RuntimeHooks& hooks, const std::size_t argc) {
    // Retail CallCFunc consumes argc VM words before invocation and then publishes one
    // return word. Host callbacks keep the same argument order without reproducing the
    // ARM calling convention. A void-style callback may return zero when its value is not
    // semantically observed by bytecode.
    const VmWord* args = ctx.stack_top ? ctx.stack_top - static_cast<std::ptrdiff_t>(argc) : nullptr;
    if (ctx.stack_top) ctx.stack_top -= static_cast<std::ptrdiff_t>(argc);
    if (hooks.invoke_native) return hooks.invoke_native(hooks.native_user,ctx,args,argc);
    if (!hooks.resolve_native) return 0;
    auto cb = hooks.resolve_native(&fn);
    return cb ? cb(ctx, args, argc) : 0;
}

bool set_function(CmContextState& ctx, const CmFunctionView& fn) {
    if (!ctx.stack_top || !fn.address) return false;

    // Retail SetFunction consumes the callee's declared argc only for ordinary script
    // functions (type 0), reserves local_words, then stores previous instruction,
    // function and frame in three logical VM words.
    VmWord* new_frame = ctx.stack_top;
    if (fn.type == 0) new_frame -= static_cast<std::ptrdiff_t>(fn.argc);
    VmWord* saved = new_frame + static_cast<std::ptrdiff_t>(fn.local_words);
    saved[0] = reinterpret_cast<VmWord>(ctx.instruction);
    saved[1] = reinterpret_cast<VmWord>(ctx.function);
    saved[2] = reinterpret_cast<VmWord>(ctx.frame_base);

    ctx.stack_top = saved + 3;
    ctx.frame_base = new_frame;
    ctx.function = &fn;
    ctx.instruction = fn.address;
    ctx.stopped = false;
    ctx.yielded = false;
    return true;
}

bool is_source_owned_opcode(const std::uint8_t opcode) noexcept {
    switch (opcode) {
    case kOpcodeLocalLoadI8:
    case kOpcodeLocalAddressI8:
    case kOpcodePushI8:
    case kOpcodePushI16Be:
    case kOpcodePushWord32Be:
    case kOpcodePushArchiveAddressI8:
    case kOpcodePushArchiveAddressI16Be:
    case kOpcodePushWord32BeAlt:
    case kOpcodeLoadIndirectKeepAddress:
    case kOpcodeDrop:
    case kOpcodeStoreIndirect:
    case kOpcodeSubtractInt:
    case kOpcodeDivideInt:
    case kOpcodeRemainderInt:
    case kOpcodeCompareEqualWord:
    case kOpcodeCompareGreaterInt:
    case kOpcodeDuplicateTopWord:
    case kOpcodeLogicalNot:
    case kOpcodeCompareLessEqualInt:
    case kOpcodeCompareGreaterEqualInt:
    case kOpcodeCallFunctionIndex:
    case kOpcodeCallIdent:
    case kOpcodeReturnTop:
    case kOpcodeBranchI16Be:
    case kOpcodeBranchIfZeroI16Be:
    case kOpcodeYield:
    case kOpcodeInlineWordSkip:
    case kOpcodeReturnFalse:
    case kOpcodeReturnTrue:
        return true;
    default:
        return false;
    }
}

StepResult step_instruction(CmContextState& ctx, const RuntimeHooks& hooks) {
    if (!ctx.instruction || ctx.stopped) return StepResult::Stop;
    ctx.yielded = false;
    const auto opcode = std::to_integer<std::uint8_t>(*ctx.instruction++);

    switch (opcode) {
    case kOpcodeLocalLoadI8: {
        if (!ctx.frame_base) return RuntimeBoundary(ctx, hooks, opcode);
        const auto index = ReadI8(ctx.instruction);
        Push(ctx, ctx.frame_base[index]);
        return StepResult::Continue;
    }
    case kOpcodeLocalAddressI8: {
        if (!ctx.frame_base) return RuntimeBoundary(ctx, hooks, opcode);
        const auto index = ReadI8(ctx.instruction);
        Push(ctx, reinterpret_cast<VmWord>(ctx.frame_base + index));
        return StepResult::Continue;
    }
    case kOpcodePushI8:
        Push(ctx, static_cast<VmWord>(ReadI8(ctx.instruction)));
        return StepResult::Continue;
    case kOpcodePushI16Be:
        Push(ctx, static_cast<VmWord>(ReadI16Be(ctx.instruction)));
        return StepResult::Continue;
    case kOpcodePushWord32Be:
    case kOpcodePushWord32BeAlt:
        Push(ctx, static_cast<VmWord>(ReadWord32Be(ctx.instruction)));
        return StepResult::Continue;
    case kOpcodePushArchiveAddressI8: {
        const auto* const base = StringBase(ctx);
        if (!base) return RuntimeBoundary(ctx, hooks, opcode);
        const auto offset = ReadI8(ctx.instruction);
        Push(ctx, reinterpret_cast<VmWord>(base + offset));
        return StepResult::Continue;
    }
    case kOpcodePushArchiveAddressI16Be: {
        const auto* const base = StringBase(ctx);
        if (!base) return RuntimeBoundary(ctx, hooks, opcode);
        const auto offset = ReadI16Be(ctx.instruction);
        Push(ctx, reinterpret_cast<VmWord>(base + offset));
        return StepResult::Continue;
    }
    case kOpcodeLoadIndirectKeepAddress:
        if (!ctx.stack_top || ctx.stack_top == ctx.stack_base) return RuntimeBoundary(ctx, hooks, opcode);
        if (!ctx.stack_top[-1]) return RuntimeBoundary(ctx, hooks, opcode);
        Push(ctx, *reinterpret_cast<const VmWord*>(ctx.stack_top[-1]));
        return StepResult::Continue;
    case kOpcodeDrop:
        (void)Pop(ctx);
        return StepResult::Continue;
    case kOpcodeStoreIndirect: {
        const VmWord value = Pop(ctx);
        auto* const address = reinterpret_cast<VmWord*>(Pop(ctx));
        if (!address) return RuntimeBoundary(ctx, hooks, opcode);
        *address = value;
        return StepResult::Continue;
    }
    case kOpcodeSubtractInt: {
        const VmWord rhs = Pop(ctx);
        const VmWord lhs = Pop(ctx);
        Push(ctx, SubI32(lhs, rhs));
        return StepResult::Continue;
    }
    case kOpcodeLogicalNot:
        Push(ctx, Pop(ctx) == 0 ? 1 : 0);
        return StepResult::Continue;
    case kOpcodeDivideInt:
    case kOpcodeRemainderInt: {
        // Retail calls signed divmod at002FFA0C; zero enters its error service.
        // The bounded owner preflights zero without advancing/consuming operands.
        if (WordAsI32(ctx.stack_top[-1]) == 0) return RuntimeBoundary(ctx,hooks,opcode);
        const auto rhs = static_cast<std::int64_t>(WordAsI32(Pop(ctx)));
        const auto lhs = static_cast<std::int64_t>(WordAsI32(Pop(ctx)));
        const auto result = opcode==kOpcodeDivideInt ? lhs/rhs : lhs%rhs;
        // Widening avoids C++ overflow for INT_MIN/-1. Retail wraps its quotient.
        Push(ctx,std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(result)));
        return StepResult::Continue;
    }
    case kOpcodeCompareEqualWord: {
        const auto rhs=Pop(ctx);const auto lhs=Pop(ctx);
        // Integer words are canonical signed32 values. Host addresses keep all
        // pointer bits; the retained VM validates their owner and lifetime.
        Push(ctx,lhs==rhs?1:0);
        return StepResult::Continue;
    }
    case kOpcodeCompareGreaterInt: {
        const auto rhs=WordAsI32(Pop(ctx));const auto lhs=WordAsI32(Pop(ctx));
        Push(ctx,lhs>rhs?1:0);
        return StepResult::Continue;
    }
    case kOpcodeDuplicateTopWord:
        if (!ctx.stack_top || ctx.stack_top==ctx.stack_base) return RuntimeBoundary(ctx,hooks,opcode);
        Push(ctx,ctx.stack_top[-1]);
        return StepResult::Continue;
    case kOpcodeCompareLessEqualInt: {
        const auto rhs = WordAsI32(Pop(ctx));
        const auto lhs = WordAsI32(Pop(ctx));
        Push(ctx, lhs <= rhs ? 1 : 0);
        return StepResult::Continue;
    }
    case kOpcodeCompareGreaterEqualInt: {
        const auto rhs = WordAsI32(Pop(ctx));
        const auto lhs = WordAsI32(Pop(ctx));
        Push(ctx, lhs >= rhs ? 1 : 0);
        return StepResult::Continue;
    }
    case kOpcodeCallFunctionIndex: {
        const auto index = ReadFunctionIndex(ctx.instruction);
        if (!ctx.function || !ctx.function->archive || (!hooks.resolved_script_call && !hooks.resolve_function_index))
            return RuntimeBoundary(ctx, hooks, opcode);
        const CmFunctionView* const target = hooks.resolved_script_call ? hooks.resolved_script_call :
            hooks.resolve_function_index(ctx.function->archive, index);
        if (!target || target->native) return RuntimeBoundary(ctx, hooks, opcode);
        return set_function(ctx, *target) ? StepResult::Continue : RuntimeBoundary(ctx, hooks, opcode);
    }
    case kOpcodeCallIdent: {
        const auto* const base = StringBase(ctx);
        if (!base) return RuntimeBoundary(ctx, hooks, opcode);
        const auto target_offset = ReadI16Be(ctx.instruction);
        const auto argc = static_cast<std::size_t>(std::to_integer<std::uint8_t>(*ctx.instruction++));
        const char* const name = reinterpret_cast<const char*>(base + target_offset);
        if (hooks.resolved_script_call && hooks.resolved_native_call) return RuntimeBoundary(ctx,hooks,opcode);
        const CmFunctionView* const target = hooks.resolved_script_call ? hooks.resolved_script_call :
            (hooks.resolved_native_call ? hooks.resolved_native_call : (hooks.resolve_function ? hooks.resolve_function(name) : nullptr));
        if (hooks.resolved_script_call && target->native) return RuntimeBoundary(ctx, hooks, opcode);
        if (hooks.resolved_native_call && !target->native) return RuntimeBoundary(ctx,hooks,opcode);
        if (!target) {
            ctx.stack_top -= static_cast<std::ptrdiff_t>(argc);
            Push(ctx, 0);
            return StepResult::Continue;
        }
        if (target->native) {
            if (!hooks.invoke_native && (!hooks.resolve_native || !hooks.resolve_native(target))) return RuntimeBoundary(ctx, hooks, opcode);
            Push(ctx, call_native(ctx, *target, hooks, argc));
            return StepResult::Continue;
        }
        return set_function(ctx, *target) ? StepResult::Continue : RuntimeBoundary(ctx, hooks, opcode);
    }
    case kOpcodeReturnTop:
        return ReturnConstant(ctx, Pop(ctx), hooks, opcode);
    case kOpcodeBranchI16Be: {
        const std::byte* const operand_start = ctx.instruction;
        const auto rel = ReadI16Be(ctx.instruction);
        ctx.instruction = operand_start + rel;
        return StepResult::Continue;
    }
    case kOpcodeBranchIfZeroI16Be: {
        const VmWord condition = Pop(ctx);
        const std::byte* const operand_start = ctx.instruction;
        const auto rel = ReadI16Be(ctx.instruction);
        if (condition == 0) ctx.instruction = operand_start + rel;
        return StepResult::Continue;
    }
    case kOpcodeYield:
        ctx.yielded = true;
        return StepResult::Yield;
    case kOpcodeInlineWordSkip:
        ctx.instruction += 4;
        return StepResult::Continue;
    case kOpcodeReturnFalse:
        return ReturnConstant(ctx, 0, hooks, opcode);
    case kOpcodeReturnTrue:
        return ReturnConstant(ctx, 1, hooks, opcode);
    default:
        if (hooks.on_invalid_opcode) hooks.on_invalid_opcode(opcode);
        return StepResult::InvalidOpcode;
    }
}

// Remaining families stay bounded until reached by original scripts. Pass154's
// all-script phase census adds signed divmod, equality, greater-than and word
// duplication after concrete live query owners expose those operations.
} // namespace fates::cmvm
