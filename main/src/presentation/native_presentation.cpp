#include "fates/presentation/native_presentation.hpp"
#include <algorithm>

namespace fates::presentation::native {
namespace {

UnitVisualIntent IntentFor(const runtime::native::UnitState& unit) noexcept {
    if (unit.defeated) return UnitVisualIntent::Defeated;
    if (unit.pair.role == runtime::native::PairRole::Partner)
        return UnitVisualIntent::PairPartner;
    if (unit.action_committed) return UnitVisualIntent::ActionCommitted;
    if (unit.has_position) return UnitVisualIntent::Ready;
    return UnitVisualIntent::Idle;
}

const UnitPresentationState* FindUnit(
    const PresentationSnapshot& snapshot,
    const std::uint16_t slot) noexcept {
    const auto it = std::find_if(
        snapshot.units.begin(), snapshot.units.end(),
        [slot](const auto& u) { return u.slot == slot; });
    return it == snapshot.units.end() ? nullptr : &*it;
}

} // namespace

PresentationSnapshot BuildPresentationSnapshot(
    const runtime::native::NativeRuntime& runtime,
    const std::uint64_t serial) {
    PresentationSnapshot out{};
    out.serial = serial;
    out.chapter_index = runtime.game.campaign.current_chapter_index;
    out.map_active = runtime.game.map_active;
    out.phase = runtime.game.phase;
    out.camera.tactical_map = runtime.game.map_active;
    const auto& field=runtime.game.field_scene;
    if(field.snapshot&&field.snapshot->world==field.owner&&field.snapshot->revision==field.revision&&
        field.snapshot->chapter==out.chapter_index&&field.snapshot->map_active==out.map_active)
        out.field=field.snapshot;

    for (std::uint16_t slot = 0; slot < runtime.game.units.size(); ++slot) {
        const auto& u = runtime.game.units[slot];
        if (!u.occupied) continue;

        UnitPresentationState p{};
        p.slot = slot;
        p.generation = runtime.game.unit_slot_generations[slot];
        p.person_id = u.person_id;
        p.job_id = u.job_id;
        p.force_type = u.force_type;
        p.visible = runtime.game.map_active && u.has_position && u.force_type<3 &&
            !u.defeated && u.pair.role!=runtime::native::PairRole::Partner;
        p.has_position = u.has_position;
        p.x = u.x;
        p.y = u.y;
        p.defeated = u.defeated;
        p.action_committed = u.action_committed;
        p.equipped_item_id = u.equipped_item_id;
        p.pair_role = u.pair.role;
        p.pair_partner_slot =
            u.pair.bound ? u.pair.partner_slot : std::uint16_t{0xFFFFu};
        p.intent = IntentFor(u);
        out.units.push_back(p);
    }
    return out;
}

std::vector<PresentationEvent> DiffPresentationSnapshots(
    const PresentationSnapshot& before,
    const PresentationSnapshot& after) {
    std::vector<PresentationEvent> out;
    if(bool(before.field)!=bool(after.field)||(before.field&&after.field&&
        (before.field->world!=after.field->world||before.field->revision!=after.field->revision)))
        out.push_back({PresentationEventType::FieldSceneChanged});

    if (before.chapter_index != after.chapter_index) {
        out.push_back({PresentationEventType::ChapterChanged});
    }
    if (before.map_active != after.map_active) {
        out.push_back({PresentationEventType::MapActivationChanged});
    }

    if(before.phase.revision!=after.phase.revision || before.phase.stage!=after.phase.stage ||
       before.phase.chapter_index!=after.phase.chapter_index ||
       before.phase.situation.active_force!=after.phase.situation.active_force ||
       before.phase.situation.human_force!=after.phase.situation.human_force ||
       before.phase.situation.turn!=after.phase.situation.turn ||
       before.phase.situation.turn_limit!=after.phase.situation.turn_limit ||
       before.phase.situation.control!=after.phase.situation.control)
        out.push_back({PresentationEventType::PhaseContextChanged});
    for (const auto& now : after.units) {
        const auto* old = FindUnit(before, now.slot);
        if (!old || old->generation!=now.generation) {
            if(old) {
                PresentationEvent gone{PresentationEventType::UnitDisappeared};gone.unit_slot=old->slot;gone.unit_generation=old->generation;
                gone.from_x=old->x;gone.from_y=old->y;out.push_back(gone);
            }
            PresentationEvent e{PresentationEventType::UnitAppeared};
            e.unit_slot = now.slot;
            e.unit_generation = now.generation;
            e.to_x = now.x; e.to_y = now.y;
            out.push_back(e);
            continue;
        }
        if(old->visible!=now.visible) {
            PresentationEvent e{PresentationEventType::UnitVisibilityChanged};e.unit_slot=now.slot;e.unit_generation=now.generation;out.push_back(e);
        }
        if (old->has_position != now.has_position ||
            old->x != now.x || old->y != now.y) {
            PresentationEvent e{PresentationEventType::UnitMoved};
            e.unit_slot = now.slot;
            e.unit_generation = now.generation;
            e.from_x = old->x; e.from_y = old->y;
            e.to_x = now.x; e.to_y = now.y;
            out.push_back(e);
        }
        if (old->action_committed != now.action_committed) {
            PresentationEvent e{PresentationEventType::UnitActionStateChanged};
            e.unit_slot = now.slot;
            e.unit_generation = now.generation;
            out.push_back(e);
        }
        if (!old->defeated && now.defeated) {
            PresentationEvent e{PresentationEventType::UnitDefeated};
            e.unit_slot = now.slot;
            e.unit_generation = now.generation;
            out.push_back(e);
        }
        if (old->equipped_item_id != now.equipped_item_id) {
            PresentationEvent e{PresentationEventType::EquippedItemChanged};
            e.unit_slot = now.slot;
            e.unit_generation = now.generation;
            e.old_item_id = old->equipped_item_id;
            e.new_item_id = now.equipped_item_id;
            out.push_back(e);
        }
    }

    for (const auto& old : before.units) {
        if (!FindUnit(after, old.slot)) {
            PresentationEvent e{PresentationEventType::UnitDisappeared};
            e.unit_slot = old.slot;
            e.unit_generation = old.generation;
            e.from_x = old.x; e.from_y = old.y;
            out.push_back(e);
        }
    }
    return out;
}

const PresentationSnapshot& PresentationBridge::Publish(
    const runtime::native::NativeRuntime& runtime) {
    auto next = BuildPresentationSnapshot(runtime, ++serial_);
    std::vector<PresentationEvent> events;
    if (has_last_) events = DiffPresentationSnapshots(last_, next);
    sink_.ConsumeSnapshot(next);
    if (!events.empty()) sink_.ConsumeEvents(events);
    last_ = std::move(next);
    has_last_ = true;
    return last_;
}

} // namespace fates::presentation::native
