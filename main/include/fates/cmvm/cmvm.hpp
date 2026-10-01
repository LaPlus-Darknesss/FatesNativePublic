#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fates::cmvm {

// Host-native semantic VM word. Retail CMVM words are 32-bit, but retail also stores
// pointers in the same stack cells. intptr_t keeps those semantic address values valid
// on a 64-bit host; numeric opcodes explicitly preserve their retail 8/16/32-bit
// narrowing/sign-extension behavior before entering a VmWord.
using VmWord = std::intptr_t;

struct CmArchiveView {
    const std::byte* base{};
    std::size_t size{};
    const std::byte* string_base{};
};

struct CmNamedScriptLibraryView {
    CmArchiveView archive{};
    const std::byte* function_table{};
    std::size_t function_count{};
};

struct CmFunctionView {
    const std::byte* address{};
    std::uint8_t type{};
    std::uint8_t argc{};
    std::uint16_t local_words{};
    const CmArchiveView* archive{};
    bool native{};
};

struct CmContextState {
    const std::byte* instruction{};
    const CmFunctionView* function{};
    VmWord* stack_base{};
    VmWord* stack_top{};
    VmWord* frame_base{};
    VmWord result{};
    bool stopped{};
    bool yielded{};
};

enum class StepResult : std::uint8_t {
    Continue,
    Yield,
    Stop,
    InvalidOpcode,
    RuntimeBoundary,
};

using NativeCallback = VmWord(*)(CmContextState&, const VmWord*, std::size_t);

struct RuntimeHooks {
    const CmFunctionView* (*resolve_function)(const char*){};
    const CmFunctionView* (*resolve_function_index)(const CmArchiveView*, std::size_t){};
    NativeCallback (*resolve_native)(const CmFunctionView*){};
    void (*on_invalid_opcode)(std::uint8_t){};
    void (*on_runtime_boundary)(std::uint8_t){};
    // Optional retained script target already resolved and preflighted for THIS
    // instruction by an owning interpreter host. Avoids thread-global callback
    // state. The host must retain this view and its archive until the callee
    // returns, including across yields: set_function stores its address.
    // Never a native callback.
    const CmFunctionView* resolved_script_call{};
    // Optional native target plus stateful host callback for this instruction.
    // Native invocation is synchronous; these objects need only cover the step.
    // The owning VM validates arguments and service availability before use.
    const CmFunctionView* resolved_native_call{};
    VmWord (*invoke_native)(void*, CmContextState&, const VmWord*, std::size_t){};
    void* native_user{};
};

bool certify_archive(const std::byte* archive, std::size_t size);
bool open_named_script_library(const std::byte* archive, std::size_t size, CmNamedScriptLibraryView& out);
bool find_named_script_function(const CmNamedScriptLibraryView&, std::string_view name, CmFunctionView& out);
bool find_script_function_by_index(const CmNamedScriptLibraryView&, std::size_t index, CmFunctionView& out);
const char* function_event_arg_string(const CmArchiveView&, const std::uint32_t* offsets, std::size_t index);
[[nodiscard]] bool is_source_owned_opcode(std::uint8_t opcode) noexcept;
StepResult step_instruction(CmContextState&, const RuntimeHooks&);
VmWord call_native(CmContextState&, const CmFunctionView&, const RuntimeHooks&, std::size_t argc);
bool set_function(CmContextState&, const CmFunctionView&);

} // namespace fates::cmvm
