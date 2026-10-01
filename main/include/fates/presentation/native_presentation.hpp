#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <vector>

namespace fates::presentation::native {

enum class UnitVisualIntent : std::uint8_t {
    Idle,
    Ready,
    ActionCommitted,
    Defeated,
    PairPartner,
};

struct UnitPresentationState {
    std::uint16_t slot{};
    std::uint64_t generation{};
    std::uint16_t person_id{};
    std::uint16_t job_id{};
    std::uint8_t force_type{};
    bool visible{};
    bool has_position{};
    std::int16_t x{};
    std::int16_t y{};
    bool defeated{};
    bool action_committed{};
    std::uint16_t equipped_item_id{};
    runtime::native::PairRole pair_role{runtime::native::PairRole::None};
    std::uint16_t pair_partner_slot{0xFFFFu};
    UnitVisualIntent intent{UnitVisualIntent::Idle};
};

struct CameraPresentationIntent {
    bool tactical_map{};
    bool follow_unit{};
    std::uint16_t follow_unit_slot{0xFFFFu};
};

struct PresentationSnapshot {
    std::uint64_t serial{};
    std::uint8_t chapter_index{};
    bool map_active{};
    std::vector<UnitPresentationState> units;
    CameraPresentationIntent camera{};
    runtime::native::TacticalPhaseContext phase{};
    std::shared_ptr<const map::native::FieldSceneSnapshot> field;
};

enum class PresentationEventType : std::uint8_t {
    ChapterChanged,
    MapActivationChanged,
    UnitAppeared,
    UnitDisappeared,
    UnitMoved,
    UnitActionStateChanged,
    UnitDefeated,
    EquippedItemChanged,
    UnitVisibilityChanged,
    PhaseContextChanged,
    FieldSceneChanged,
};

struct PresentationEvent {
    PresentationEventType type{};
    std::uint16_t unit_slot{0xFFFFu};
    std::int16_t from_x{};
    std::int16_t from_y{};
    std::int16_t to_x{};
    std::int16_t to_y{};
    std::uint16_t old_item_id{};
    std::uint16_t new_item_id{};
    std::uint64_t unit_generation{}; // identifies the subject even after slot reuse
};

PresentationSnapshot BuildPresentationSnapshot(
    const runtime::native::NativeRuntime& runtime,
    std::uint64_t serial = 0);

std::vector<PresentationEvent> DiffPresentationSnapshots(
    const PresentationSnapshot& before,
    const PresentationSnapshot& after);

class PresentationSink {
public:
    virtual ~PresentationSink() = default;
    virtual void ConsumeSnapshot(const PresentationSnapshot& snapshot) = 0;
    virtual void ConsumeEvents(const std::vector<PresentationEvent>& events) = 0;
};

class NullPresentationSink final : public PresentationSink {
public:
    void ConsumeSnapshot(const PresentationSnapshot&) override {}
    void ConsumeEvents(const std::vector<PresentationEvent>&) override {}
};

class PresentationBridge {
public:
    explicit PresentationBridge(PresentationSink& sink) : sink_(sink) {}

    const PresentationSnapshot& Publish(
        const runtime::native::NativeRuntime& runtime);

    const PresentationSnapshot& last_snapshot() const noexcept {
        return last_;
    }

private:
    PresentationSink& sink_;
    PresentationSnapshot last_{};
    bool has_last_{};
    std::uint64_t serial_{};
};

} // namespace fates::presentation::native
