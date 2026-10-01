#pragma once
#include "fates/runtime/native_person_archives.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fates::runtime::native {

// Immutable host-native definitions parsed from original FE14 data. These are
// semantic records, not replicas of retail Person/Job/Item/etc. object layouts.
struct ChapterDefinition {
    std::string cid;
    std::string battlefield;
    std::uint8_t id{};
    std::uint8_t type{};
    std::array<std::uint8_t,3> next_chapter{};       // Birthright/Conquest/Revelation
    std::array<std::uint8_t,3> requirement{};
    std::uint16_t married_person_id{};
    std::uint8_t offspring_seal_level{};
    std::uint8_t offspring_seal_level_2{};
    std::uint8_t route_mask{};
    std::array<std::uint8_t,3> raw_0x15{};
    bool battlefield_present{true}; // original null pointer is distinct from empty text
    std::uint8_t battle_prep_screen{};
};

struct PersonDefinition {
    std::array<std::uint8_t,8> bitflags{};
    std::string pid, fid, aid, name_message, description_message;
    bool attack_stance_bonuses_present{};
    std::array<std::int8_t,20> attack_stance_increments{}; // five rank rows, four signed increments
    bool guard_stance_bonuses_present{};
    std::array<std::int8_t,40> guard_stance_increments{}; // five rows of eight signed increments
    std::uint16_t id{};
    std::uint8_t support_route{};
    std::uint8_t army{};
    std::uint16_t replacing{}, parent{}, class_1{}, class_2{};
    std::int16_t support_id{};
    std::int8_t level{}, internal_level{};
    std::uint8_t enemy_flag{};
    std::uint8_t collective_level_offset{};
    std::int8_t normal_auto_growth_offset{};
    std::int8_t lunatic_auto_growth_offset{};
    std::array<std::uint8_t,8> bases{}, growths{}, modifiers{}, penalties{}, bonuses{};
    std::array<std::uint8_t,8> weapon_exp{};
    std::array<std::uint16_t,5> skills{};
    std::array<std::uint8_t,2> skill_flags{};
    std::array<std::uint16_t,3> personal_skills{};
    std::array<std::uint16_t,2> reclasses{};
    std::uint8_t level_cap{}, body_type{};
};

// Ordered original Reliance rows; Paragon calls the threshold word "type".
struct PersonalityDefinition {
    std::uint32_t id{}; // identity metadata; retail lookup uses the array index
    std::int8_t attack_stance_capability{-1};
    std::array<std::uint8_t,3> guard_stance_capabilities{}; // original GetEdit reads these as unsigned indices
    std::array<std::int8_t,8> boon_bases{},bane_bases{},boon_modifiers{},bane_modifiers{};
};
struct RelianceDefinition {
    std::uint16_t owner_support_id{}, character_id{}, id{};
    std::array<std::uint8_t,4> thresholds{}, tag{};
};
struct JobDefinition {
    std::array<std::uint8_t,8> bitflags{};
    std::string jid, fid, name_message, description_message, movement_sound;
    std::uint16_t id{};
    std::array<std::uint8_t,2> special_flags{};
    std::array<std::uint8_t,8> bases{}, player_growths{}, enemy_growths{}, max_stats{}, pair_up_bonuses{};
    std::array<std::uint8_t,8> max_weapon_exp{};
    std::int16_t hit{}, crit{}, avoid{}, dodge{};
    std::array<std::uint16_t,4> skills{};
    std::uint8_t movement_cost_index{};
    std::uint8_t movement{};
    std::uint8_t smoke_cloud_size{};
    std::uint8_t extra_flags{};
    std::array<std::uint16_t,2> advanced_classes{}, base_classes{};
    std::uint16_t gender_equivalent{}, parallel_class{};
    std::uint8_t origin{};
    std::uint8_t dlc_skill_index{};
};

struct ItemSubKindDefinition {
    std::uint8_t raw_category{};
    std::uint8_t weapon_exp_group{};
    std::uint8_t raw_02{};
    std::uint8_t raw_03{};
    std::string mikid, mikid_h;
    std::uint32_t raw_flags{};
};

struct ItemDefinition {
    std::array<std::uint8_t,8> bitflags{};
    std::string iid, name_message, description_message, com;
    std::uint16_t id{}, list_position{}, icon{};
    std::uint8_t weapon_category{}, non_weapon_category{}, required_weapon_exp{}, base_staff_exp{};
    std::int8_t uses{}, might{}, min_range{}, max_range{}, effective_speed_player{}, effective_speed_enemy{};
    std::int16_t hit{}, crit{}, avoid{}, dodge{};
    std::int32_t buy_price{}, sell_price{};
    std::array<std::uint8_t,2> effective_damage{};
    std::uint8_t movement{}, legal_player_weapon{};
    std::array<std::uint8_t,8> bonuses{}, extra_data{};
    std::uint8_t forge_table_index{}, raw_0x49{}, skirmish_item_drop{}, raw_0x4b{};
};

struct ForgeEntryDefinition {
    std::uint8_t forged_level{},power{},hit{},critical{};
};

struct SkillDefinition {
    std::string seid, name_message, description_message, effect;
    std::uint16_t id{};
    std::int16_t sort_order{};
    std::uint16_t icon{};
    std::uint8_t stat{}, trigger_factor{}, trigger_divisor{}, flags_1{};
    std::int16_t base_price{};
    std::uint8_t flags_2{};
};

struct TerrainDefinition {
    std::string tid;
    std::uint8_t id{}, change_id_1{}, change_id_2{}; // Paragon Tile +0x0C/+0x0D/+0x0E
    std::uint8_t movement_cost_index{};
    std::int8_t defense_bonus{};
    std::int8_t avoid_bonus{};
    std::int8_t healing_bonus{};
    std::uint32_t flags_0x18{};
};

struct WeaponBonusDefinition {
    std::array<std::int8_t,6> might{}; // S,A,B,C,D,E
    std::array<std::int8_t,6> hit{};
};

struct WeaponInteractionDefinition {
    WeaponBonusDefinition by_rank{};
    std::array<std::array<std::int8_t,8>,8> delta{};
};

struct TerrainMapDefinition {
    std::uint32_t width{}, height{};
    std::uint32_t min_x{}, min_y{}, max_x{}, max_y{};
    std::vector<TerrainDefinition> terrain_types;
    // FE14 terrain grids use a 32-cell row stride; the shipped backing grid is
    // 32x32 even when the active map is smaller.
    std::array<std::uint8_t,1024> grid{};
};

struct AiCommandDefinition {
    std::uint8_t kind{}, subtype{};
    std::int8_t activity_selector{};
    std::uint8_t flags{};
    std::array<std::int16_t,2> arguments{{-1,-1}};
};
struct AiDeclarationDefinition {
    std::string name;
    std::uint8_t id{}, channel{};
    // Retain the terminating kind-zero record. Metadata id is not the table index.
    std::vector<AiCommandDefinition> commands;
};

// Original named flag records, resolved from the current GameData archive.
// Personal restriction order is Player, Takumi, Ryoma, Leo, Xander, Ophelia.
struct MovementRuleDefinitions {
    std::uint64_t movement_prohibited{},personal_terrain_cost{};
    std::uint64_t held_enhance{},fujin{},brynhildr{};
    std::array<std::uint64_t,6> person_restrictions{},item_restrictions{};
    std::uint16_t shooter_category{},flying_category{};
};
struct ItemRangeRuleDefinitions {
    std::uint64_t basic_staff{},interference_staff{},recovery_staff{};
};
constexpr bool IsMagicItemGroupExact(std::uint8_t group,bool magic_weapon_flag) noexcept {
    return group==5u||group==6u||magic_weapon_flag;
}

class DefinitionStore {
public:
    // Read-only projection of original AIData. Numeric/null/AIDesc-name values;
    // other SetValue forms require another provider and are rejected atomically.
    bool LoadAiArchive(std::span<const std::uint8_t> fe14_wrapped_lz11);
    const AiDeclarationDefinition* FindAiDeclaration(std::uint8_t channel, std::uint8_t table_index) const;
    const std::array<std::vector<AiDeclarationDefinition>,4>& ai_declarations() const noexcept { return ai_declarations_; }
    bool ai_declarations_loaded() const noexcept { return ai_loaded_; }
    bool LoadCoreGameData(std::span<const std::uint8_t> fe14_wrapped_lz11);
    bool LoadPersonArchive(std::span<const std::uint8_t> fe14_wrapped_lz11);
    const std::vector<PersonArchiveDescriptor>& person_archives() const noexcept { return person_archives_; }
    bool person_archives_known() const noexcept { return person_archives_known_; }
    PersonRecordReference RetainPerson(const PersonDefinition&) const noexcept;
    const PersonDefinition* ResolvePerson(const PersonRecordReference&) const noexcept;
    // Bounded Person-name projection. nullopt means the shared archive hash
    // owner is required; an empty reference means known absence. Never guess
    // bucket count, collision order, or the lifetime of a detached archive.
    std::optional<PersonRecordReference> ResolveUnambiguousPersonName(std::string_view) const noexcept;
    std::optional<bool> IsPersonDownload(const PersonDefinition&) const noexcept;
    const PersonDefinition* GetPersonOrFirst(std::uint16_t id) const;
    PersonArchivePlan RemovePersonArchive(std::string_view name,bool destroy_owner=true);
    PersonArchivePlan PrunePersonArchives();
    PersonArchivePlan FinalizePersonArchives();
    bool LoadTerrainArchive(std::span<const std::uint8_t> fe14_wrapped_lz11);

    const PersonalityDefinition* FindPersonality(std::uint8_t index) const { return index<personalities_.size()?&personalities_[index]:nullptr; }
    const std::vector<PersonalityDefinition>& personalities() const noexcept { return personalities_; }
    const RelianceDefinition* FindReliance(std::uint16_t subject_person, std::uint16_t other_person) const;
    const std::vector<RelianceDefinition>& reliance_records() const noexcept { return reliance_records_; }
    bool support_tables_loaded() const noexcept { return support_tables_loaded_; }
    std::uint64_t support_revision() const noexcept { return support_revision_; }
    const ChapterDefinition* FindChapter(std::string_view cid) const;
    const ChapterDefinition* FindChapter(std::uint8_t id) const;
    const PersonDefinition* FindPerson(std::string_view pid) const;
    const PersonDefinition* FindPerson(std::uint16_t id) const;
    const JobDefinition* FindJob(std::string_view jid) const;
    const JobDefinition* FindJob(std::uint16_t id) const;
    // FE14 Dispo Spawn.class is a nullable Job reference. Retail CreateFromDispos
    // passes null through to CreateImpl, which then resolves Person.class_1.
    const JobDefinition* ResolveDisposJob(
        const PersonDefinition& person, std::string_view authored_jid) const;
    const ItemDefinition* FindItem(std::string_view iid) const;
    const ItemDefinition* FindItem(std::uint16_t id) const;
    const ForgeEntryDefinition* FindForgeEntry(std::uint8_t table,std::uint8_t rank) const noexcept;
    const std::array<ForgeEntryDefinition,30>& forge_entries() const noexcept {return forge_entries_;}
    bool forge_tables_loaded() const noexcept {return forge_loaded_;}
    const SkillDefinition* FindSkill(std::string_view seid) const;
    const SkillDefinition* FindSkill(std::uint16_t id) const;
    const ItemSubKindDefinition* FindItemSubKind(std::uint8_t raw_category) const;
    std::uint8_t WeaponExpGroupForItem(const ItemDefinition& item) const noexcept;
    bool IsMagicItem(const ItemDefinition& item) const noexcept;
    std::uint16_t BaseJobCategoryMask(const JobDefinition& job) const noexcept;

    const TerrainMapDefinition* terrain_map() const noexcept { return terrain_loaded_ ? &terrain_ : nullptr; }
    const TerrainDefinition* TerrainAt(std::uint32_t x, std::uint32_t y) const;
    const std::vector<std::vector<std::int8_t>>& movement_costs() const noexcept { return movement_costs_; }
    const std::array<std::uint8_t,6>& weapon_rank_thresholds() const noexcept { return weapon_rank_thresholds_; }
    std::optional<std::uint64_t> pair_prohibition_mask() const noexcept {return pair_prohibition_mask_;}
    std::optional<std::uint64_t> protagonist_mask() const noexcept {return protagonist_mask_;}
    const std::optional<MovementRuleDefinitions>& movement_rules() const noexcept {return movement_rules_;}
    const std::optional<ItemRangeRuleDefinitions>& item_range_rules() const noexcept {return item_range_rules_;}
    int WeaponRankIndex(std::uint8_t exp) const noexcept;
    std::int8_t WeaponRankMightBonus(std::uint8_t weapon_category, std::uint8_t exp) const noexcept;
    std::int8_t WeaponRankHitBonus(std::uint8_t weapon_category, std::uint8_t exp) const noexcept;
    std::int8_t WeaponInteractionDelta(std::uint8_t attacker_category, std::uint8_t defender_category) const noexcept;
    std::int8_t WeaponInteractionMight(std::uint8_t rank_index) const noexcept;
    std::int8_t WeaponInteractionHit(std::uint8_t rank_index) const noexcept;

    const std::vector<ChapterDefinition>& chapters() const noexcept { return chapters_; }
    const std::vector<PersonDefinition>& people() const noexcept { return people_; }
    const std::vector<JobDefinition>& jobs() const noexcept { return jobs_; }
    const std::vector<ItemDefinition>& items() const noexcept { return items_; }
    const std::vector<SkillDefinition>& skills() const noexcept { return skills_; }
    const std::array<ItemSubKindDefinition,21>& item_subkinds() const noexcept { return item_subkinds_; }

private:
    std::optional<std::uint64_t> pair_prohibition_mask_;
    std::optional<std::uint64_t> protagonist_mask_;
    std::optional<MovementRuleDefinitions> movement_rules_;
    std::optional<ItemRangeRuleDefinitions> item_range_rules_;
    struct PersonArchiveStorage {
        std::uint32_t token{};
        std::size_t people_begin{},people_count{},reliance_begin{},reliance_count{};
        std::shared_ptr<const PersonRecordReference::Instance> identity;
    };
    std::vector<PersonArchiveDescriptor> person_archives_;
    std::vector<PersonArchiveStorage> person_archive_storage_;
    bool person_archives_known_{};
    std::unordered_set<std::uint32_t> person_name_unknown_hashes_;
    bool person_name_metadata_known_{};
    std::uint32_t next_person_archive_token_{1};
    void RebuildPersonNames();
    void RecordPersonNameAmbiguities();
    void RecordPersonNameAuthority(const std::vector<std::uint8_t>&,std::size_t person_table);
    PersonArchivePlan ChangePersonArchives(PersonArchiveOperation,std::string_view name={});
    const PersonDefinition* PersonAtLocation(const PersonArchiveLocation&) const;
    std::array<std::vector<AiDeclarationDefinition>,4> ai_declarations_;
    bool ai_loaded_{};
    template<class T> using NameIndex=std::unordered_map<std::string,std::size_t>;
    std::vector<PersonalityDefinition> personalities_;
    std::vector<RelianceDefinition> reliance_records_;
    bool support_tables_loaded_{};
    std::uint64_t support_revision_{};
    std::vector<ChapterDefinition> chapters_;
    std::vector<PersonDefinition> people_;
    std::vector<JobDefinition> jobs_;
    std::vector<ItemDefinition> items_;
    std::vector<SkillDefinition> skills_;
    std::array<ForgeEntryDefinition,30> forge_entries_{};
    bool forge_loaded_{};
    std::array<ItemSubKindDefinition,21> item_subkinds_{};
    std::vector<std::vector<std::int8_t>> movement_costs_;
    std::array<std::uint8_t,6> weapon_rank_thresholds_{};
    std::array<WeaponBonusDefinition,8> weapon_bonuses_{};
    WeaponInteractionDefinition weapon_interaction_{};
    NameIndex<ChapterDefinition> chapter_by_name_;
    NameIndex<PersonDefinition> person_by_name_;
    NameIndex<JobDefinition> job_by_name_;
    NameIndex<ItemDefinition> item_by_name_;
    NameIndex<SkillDefinition> skill_by_name_;
    std::unordered_map<std::uint16_t,std::size_t> person_by_id_,job_by_id_,item_by_id_,skill_by_id_;
    std::array<std::size_t,256> chapter_by_id_{};
    std::array<bool,256> chapter_id_present_{};
    TerrainMapDefinition terrain_{};
    bool terrain_loaded_{};
};

} // namespace fates::runtime::native
