#include "fates/runtime/native_definition_store.hpp"
#include "fates/runtime/native_archive.hpp"
#include "fates/runtime/native_identifier.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace fates::runtime::native {
namespace {

constexpr std::size_t kPointerBase = 0x20;

std::uint16_t ReadU16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) {
        throw 1;
    }
    return static_cast<std::uint16_t>(
        bytes[offset] | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8));
}

std::int16_t ReadS16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::int16_t>(ReadU16(bytes, offset));
}

std::uint32_t ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 4 > bytes.size()) {
        throw 1;
    }
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::int32_t ReadS32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::int32_t>(ReadU32(bytes, offset));
}

std::string ReadString(const std::vector<std::uint8_t>& bytes, std::uint32_t relative) {
    if (relative == 0) {
        return {};
    }
    const std::size_t offset = kPointerBase + relative;
    if (offset >= bytes.size()) {
        throw 1;
    }
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(offset);
    const auto end = std::find(begin, bytes.end(), 0);
    if (end == bytes.end()) {
        throw 1;
    }
    // Retail identifiers and message IDs are ASCII plus Shift-JIS bytes.
    // Preserve those original bytes verbatim; localization is a higher layer.
    return std::string(
        reinterpret_cast<const char*>(bytes.data() + offset),
        static_cast<std::size_t>(end - begin));
}

std::size_t ReadRootPointer(const std::vector<std::uint8_t>& bytes, std::size_t root_offset) {
    return kPointerBase + ReadU32(bytes, kPointerBase + root_offset);
}



template<std::size_t N>
std::array<std::uint8_t, N> ReadBytes(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    if (offset + N > bytes.size()) {
        throw 1;
    }
    std::array<std::uint8_t, N> result{};
    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        N,
        result.begin());
    return result;
}

std::vector<RelianceDefinition> ParseReliance(const std::vector<std::uint8_t>& bytes, std::size_t person_table) {
    std::vector<RelianceDefinition> result;
    const auto root=ReadU32(bytes,person_table+8);
    if(!root)return result; // a known null table is distinct from unparsed input
    const std::size_t at=kPointerBase+root;
    const auto count=ReadU32(bytes,at);
    if(at>bytes.size() || bytes.size()-at<4 || count>(bytes.size()-at-4)/4)throw 1;
    for(std::size_t i=0;i<count;++i) {
        const auto ptr=ReadU32(bytes,at+4+i*4);if(!ptr)throw 1;
        const std::size_t table=kPointerBase+ptr;
        const auto owner=ReadU16(bytes,table), rows=ReadU16(bytes,table+2);
        if(table>bytes.size() || bytes.size()-table<4 || rows>(bytes.size()-table-4)/12)throw 1;
        for(std::size_t j=0;j<rows;++j) {
            const auto r=table+4+j*12;
            result.push_back({owner,ReadU16(bytes,r),ReadU16(bytes,r+2),ReadBytes<4>(bytes,r+4),ReadBytes<4>(bytes,r+8)});
        }
    }
    return result;
}

PersonArchiveDescriptor ParsePersonArchive(const std::vector<std::uint8_t>& bytes,
    std::size_t table,std::uint32_t token,bool owner) {
    PersonArchiveDescriptor out{};out.token=token;out.has_owner=owner;
    const auto name=ReadU32(bytes,table);if(name)out.name=ReadString(bytes,name);
    out.count=ReadU16(bytes,table+4);out.resident=bytes.at(table+7);
    out.has_support=ReadU32(bytes,table+8)!=0;
    if(out.count)out.first_id=ReadU16(bytes,table+0x34);
    return out;
}

PersonDefinition ParsePerson(const std::vector<std::uint8_t>& bytes, std::size_t record) {
    PersonDefinition person{};
    person.bitflags = ReadBytes<8>(bytes, record);
    person.pid = ReadString(bytes, ReadU32(bytes, record + 0x08));
    person.fid = ReadString(bytes, ReadU32(bytes, record + 0x0C));
    person.aid = ReadString(bytes, ReadU32(bytes, record + 0x10));
    person.name_message = ReadString(bytes, ReadU32(bytes, record + 0x14));
    person.description_message = ReadString(bytes, ReadU32(bytes, record + 0x18));
    const auto support_pointer=ReadU32(bytes,record+0x1C);
    if(support_pointer) {
        const auto raw=ReadBytes<20>(bytes,kPointerBase+support_pointer);
        person.attack_stance_bonuses_present=true;
        for(std::size_t i=0;i<raw.size();++i)person.attack_stance_increments[i]=static_cast<std::int8_t>(raw[i]);
    }
    const auto guard_pointer=ReadU32(bytes,record+0x20);
    if(guard_pointer) {
        const auto raw=ReadBytes<40>(bytes,kPointerBase+guard_pointer);
        person.guard_stance_bonuses_present=true;
        for(std::size_t i=0;i<raw.size();++i)person.guard_stance_increments[i]=static_cast<std::int8_t>(raw[i]);
    }
    person.id = ReadU16(bytes, record + 0x24);
    person.support_route = bytes.at(record + 0x26);
    person.army = bytes.at(record + 0x27);
    person.replacing = ReadU16(bytes, record + 0x28);
    person.parent = ReadU16(bytes, record + 0x2A);
    person.class_1 = ReadU16(bytes, record + 0x2C);
    person.class_2 = ReadU16(bytes, record + 0x2E);
    person.support_id = ReadS16(bytes, record + 0x30);
    person.level = static_cast<std::int8_t>(bytes.at(record + 0x32));
    person.internal_level = static_cast<std::int8_t>(bytes.at(record + 0x33));
    person.enemy_flag = bytes.at(record + 0x34);
    person.collective_level_offset = bytes.at(record + 0x35);
    person.normal_auto_growth_offset = static_cast<std::int8_t>(bytes.at(record + 0x36));
    person.lunatic_auto_growth_offset = static_cast<std::int8_t>(bytes.at(record + 0x37));
    person.bases = ReadBytes<8>(bytes, record + 0x38);
    person.growths = ReadBytes<8>(bytes, record + 0x40);
    person.modifiers = ReadBytes<8>(bytes, record + 0x48);
    person.penalties = ReadBytes<8>(bytes, record + 0x50);
    person.bonuses = ReadBytes<8>(bytes, record + 0x58);
    person.weapon_exp = ReadBytes<8>(bytes, record + 0x60);
    for (int i = 0; i < 5; ++i) {
        person.skills[i] = ReadU16(bytes, record + 0x68 + static_cast<std::size_t>(i) * 2);
    }
    person.skill_flags = ReadBytes<2>(bytes, record + 0x72);
    for (int i = 0; i < 3; ++i) {
        person.personal_skills[i] = ReadU16(bytes, record + 0x74 + static_cast<std::size_t>(i) * 2);
    }
    person.reclasses[0] = ReadU16(bytes, record + 0x7C);
    person.reclasses[1] = ReadU16(bytes, record + 0x7E);
    person.level_cap = bytes.at(record + 0x86);
    person.body_type = bytes.at(record + 0x87);
    return person;
}

JobDefinition ParseJob(const std::vector<std::uint8_t>& bytes, std::size_t record) {
    JobDefinition job{};
    job.bitflags = ReadBytes<8>(bytes, record);
    job.jid = ReadString(bytes, ReadU32(bytes, record + 0x08));
    job.fid = ReadString(bytes, ReadU32(bytes, record + 0x0C));
    job.name_message = ReadString(bytes, ReadU32(bytes, record + 0x10));
    job.description_message = ReadString(bytes, ReadU32(bytes, record + 0x14));
    job.id = ReadU16(bytes, record + 0x18);
    job.special_flags = ReadBytes<2>(bytes, record + 0x1A);
    job.bases = ReadBytes<8>(bytes, record + 0x1C);
    job.player_growths = ReadBytes<8>(bytes, record + 0x24);
    job.enemy_growths = ReadBytes<8>(bytes, record + 0x2C);
    job.max_stats = ReadBytes<8>(bytes, record + 0x34);
    job.pair_up_bonuses = ReadBytes<8>(bytes, record + 0x3C);
    job.max_weapon_exp = ReadBytes<8>(bytes, record + 0x44);
    job.hit = ReadS16(bytes, record + 0x4C);
    job.crit = ReadS16(bytes, record + 0x4E);
    job.avoid = ReadS16(bytes, record + 0x50);
    job.dodge = ReadS16(bytes, record + 0x52);
    for (int i = 0; i < 4; ++i) {
        job.skills[i] = ReadU16(bytes, record + 0x54 + static_cast<std::size_t>(i) * 2);
    }
    job.movement_cost_index = bytes.at(record + 0x5C);
    job.movement = bytes.at(record + 0x5D);
    job.smoke_cloud_size = bytes.at(record + 0x5E);
    job.extra_flags = bytes.at(record + 0x5F);
    job.movement_sound = ReadString(bytes, ReadU32(bytes, record + 0x60));
    job.advanced_classes = {ReadU16(bytes, record + 0x64), ReadU16(bytes, record + 0x66)};
    job.base_classes = {ReadU16(bytes, record + 0x68), ReadU16(bytes, record + 0x6A)};
    job.gender_equivalent = ReadU16(bytes, record + 0x70);
    job.parallel_class = ReadU16(bytes, record + 0x72);
    job.origin = bytes.at(record + 0x74);
    job.dlc_skill_index = bytes.at(record + 0x7B);
    return job;
}

ItemDefinition ParseItem(const std::vector<std::uint8_t>& bytes, std::size_t record) {
    ItemDefinition item{};
    item.bitflags = ReadBytes<8>(bytes, record);
    item.iid = ReadString(bytes, ReadU32(bytes, record + 0x08));
    item.name_message = ReadString(bytes, ReadU32(bytes, record + 0x0C));
    item.description_message = ReadString(bytes, ReadU32(bytes, record + 0x10));
    item.id = ReadU16(bytes, record + 0x14);
    item.list_position = ReadU16(bytes, record + 0x16);
    item.icon = ReadU16(bytes, record + 0x18);
    item.weapon_category = bytes.at(record + 0x1A);
    item.non_weapon_category = bytes.at(record + 0x1B);
    item.required_weapon_exp = bytes.at(record + 0x1C);
    item.base_staff_exp = bytes.at(record + 0x1D);
    item.uses = static_cast<std::int8_t>(bytes.at(record + 0x1E));
    item.might = static_cast<std::int8_t>(bytes.at(record + 0x1F));
    item.hit = ReadS16(bytes, record + 0x20);
    item.crit = ReadS16(bytes, record + 0x22);
    item.avoid = ReadS16(bytes, record + 0x24);
    item.dodge = ReadS16(bytes, record + 0x26);
    item.min_range = static_cast<std::int8_t>(bytes.at(record + 0x28));
    item.max_range = static_cast<std::int8_t>(bytes.at(record + 0x29));
    item.effective_speed_player = static_cast<std::int8_t>(bytes.at(record + 0x2A));
    item.effective_speed_enemy = static_cast<std::int8_t>(bytes.at(record + 0x2B));
    item.buy_price = ReadS32(bytes, record + 0x2C);
    item.sell_price = ReadS32(bytes, record + 0x30);
    item.effective_damage = ReadBytes<2>(bytes, record + 0x34);
    item.movement = bytes.at(record + 0x36);
    item.legal_player_weapon = bytes.at(record + 0x37);
    item.bonuses = ReadBytes<8>(bytes, record + 0x38);
    item.extra_data = ReadBytes<8>(bytes, record + 0x40);
    item.forge_table_index = bytes.at(record + 0x48);
    item.raw_0x49 = bytes.at(record + 0x49);
    item.skirmish_item_drop = bytes.at(record + 0x4A);
    item.raw_0x4b = bytes.at(record + 0x4B);
    item.com = ReadString(bytes, ReadU32(bytes, record + 0x60));
    return item;
}

SkillDefinition ParseSkill(const std::vector<std::uint8_t>& bytes, std::size_t record) {
    SkillDefinition skill{};
    skill.seid = ReadString(bytes, ReadU32(bytes, record + 0x00));
    skill.name_message = ReadString(bytes, ReadU32(bytes, record + 0x04));
    skill.description_message = ReadString(bytes, ReadU32(bytes, record + 0x08));
    skill.effect = ReadString(bytes, ReadU32(bytes, record + 0x0C));
    skill.id = ReadU16(bytes, record + 0x10);
    skill.sort_order = ReadS16(bytes, record + 0x12);
    skill.icon = ReadU16(bytes, record + 0x14);
    skill.stat = bytes.at(record + 0x16);
    skill.trigger_factor = bytes.at(record + 0x17);
    skill.trigger_divisor = bytes.at(record + 0x18);
    skill.flags_1 = bytes.at(record + 0x19);
    skill.base_price = ReadS16(bytes, record + 0x1A);
    skill.flags_2 = bytes.at(record + 0x1C);
    return skill;
}

template<class Vector, class Map>
const typename Vector::value_type* FindByName(
    const Vector& values,
    const Map& map,
    std::string_view key) {
    const auto it = map.find(std::string(key));
    return it == map.end() ? nullptr : &values[it->second];
}

template<class Vector, class Map, class Key>
const typename Vector::value_type* FindById(
    const Vector& values,
    const Map& map,
    Key key) {
    const auto it = map.find(key);
    return it == map.end() ? nullptr : &values[it->second];
}

} // namespace

const ForgeEntryDefinition* DefinitionStore::FindForgeEntry(std::uint8_t table,std::uint8_t rank) const noexcept {
    // ItemRefine::Get computes one flat index; it does not clamp the rank.
    const auto index=std::size_t(table)*10u+rank;
    return forge_loaded_ && index<forge_entries_.size()?&forge_entries_[index]:nullptr;
}

bool DefinitionStore::LoadCoreGameData(std::span<const std::uint8_t> raw) {
    ++support_revision_;support_tables_loaded_=false;forge_loaded_=false;person_archives_known_=false;pair_prohibition_mask_.reset();protagonist_mask_.reset();movement_rules_.reset();item_range_rules_.reset();
    if(!next_person_archive_token_)return false;
    std::vector<std::uint8_t> bytes;
    if (!DecompressFe14Archive(raw, bytes) || bytes.size() < 0x84) {
        return false;
    }

    try {
        // Shift-JIS identity used by original Unit::CanDoubleOn / PrivateSkill::Get.
        const auto pair_bit=ReadArchiveLabeledWord(bytes,"SPID_\x83\x5f\x83\x75\x83\x8b\x8b\xd6\x8e\x7e");
        const auto protagonist_bit=ReadArchiveLabeledWord(bytes,"SPID_\x8e\xe5\x90\x6c\x8c\xf6");
        MovementRuleDefinitions movement_rules{};bool movement_rules_known=true;
        auto mask=[&](const char* label)->std::uint64_t {
            const auto bit=ReadArchiveLabeledWord(bytes,label);
            if(!bit||*bit>=64){movement_rules_known=false;return 0;}
            return std::uint64_t(1)<<*bit;
        };
        movement_rules.movement_prohibited=mask("\x53\x50\x49\x44\x5f\x88\xda\x93\xae\x8b\xd6\x8e\x7e");
        movement_rules.personal_terrain_cost=mask("\x53\x50\x49\x44\x5f\x90\xea\x97\x70\x92\x6e\x8c\x60\x83\x52\x83\x58\x83\x67");
        movement_rules.held_enhance=mask("\x49\x53\x49\x44\x5f\x8f\x8a\x8e\x9d\x8b\xad\x89\xbb");
        movement_rules.fujin=mask("\x49\x53\x49\x44\x5f\x95\x97\x90\x5f\x8b\x7c");
        movement_rules.brynhildr=mask("\x49\x53\x49\x44\x5f\x83\x75\x83\x8a\x83\x85\x83\x93\x83\x71\x83\x8b\x83\x66");
        movement_rules.person_restrictions[0]=mask("\x53\x50\x49\x44\x5f\x83\x76\x83\x8c\x83\x43\x83\x84\x81\x5b");
        movement_rules.person_restrictions[1]=mask("\x53\x50\x49\x44\x5f\x83\x5e\x83\x4e\x83\x7e");
        movement_rules.person_restrictions[2]=mask("\x53\x50\x49\x44\x5f\x83\x8a\x83\x87\x83\x45\x83\x7d");
        movement_rules.person_restrictions[3]=mask("\x53\x50\x49\x44\x5f\x83\x8c\x83\x49\x83\x93");
        movement_rules.person_restrictions[4]=mask("\x53\x50\x49\x44\x5f\x83\x7d\x81\x5b\x83\x4e\x83\x58");
        movement_rules.person_restrictions[5]=mask("\x53\x50\x49\x44\x5f\x83\x49\x83\x74\x83\x46\x83\x8a\x83\x41");
        movement_rules.item_restrictions[0]=mask("\x49\x53\x49\x44\x5f\x83\x76\x83\x8c\x83\x43\x83\x84\x81\x5b\x90\xea\x97\x70");
        movement_rules.item_restrictions[1]=mask("\x49\x53\x49\x44\x5f\x83\x5e\x83\x4e\x83\x7e\x90\xea\x97\x70");
        movement_rules.item_restrictions[2]=mask("\x49\x53\x49\x44\x5f\x83\x8a\x83\x87\x83\x45\x83\x7d\x90\xea\x97\x70");
        movement_rules.item_restrictions[3]=mask("\x49\x53\x49\x44\x5f\x83\x8c\x83\x49\x83\x93\x90\xea\x97\x70");
        movement_rules.item_restrictions[4]=mask("\x49\x53\x49\x44\x5f\x83\x7d\x81\x5b\x83\x4e\x83\x58\x90\xea\x97\x70");
        movement_rules.item_restrictions[5]=mask("\x49\x53\x49\x44\x5f\x83\x49\x83\x74\x83\x46\x83\x8a\x83\x41\x90\xea\x97\x70");
        const auto shooter_bit=ReadArchiveLabeledWord(bytes,"\x4a\x43\x49\x44\x5f\x83\x56\x83\x85\x81\x5b\x83\x5e\x81\x5b");
        if(shooter_bit&&*shooter_bit<16)movement_rules.shooter_category=std::uint16_t(1u<<*shooter_bit);
        else movement_rules_known=false;
        const auto flying_bit=ReadArchiveLabeledWord(bytes,"JCID_\x94\xf2\x8d\x73");
        if(flying_bit&&*flying_bit<16)movement_rules.flying_category=std::uint16_t(1u<<*flying_bit);
        else movement_rules_known=false;
        chapters_.clear();
        ItemRangeRuleDefinitions item_ranges{};bool item_ranges_known=true;
        auto item_range_mask=[&](const char* label)->std::uint64_t {
            const auto bit=ReadArchiveLabeledWord(bytes,label);
            if(!bit||*bit>=64){item_ranges_known=false;return 0;}return std::uint64_t(1)<<*bit;
        };
        item_ranges.basic_staff=item_range_mask("ISID_\x8a\xee\x96\x7b\x8f\xf1");
        item_ranges.interference_staff=item_range_mask("ISID_\x96\x57\x8a\x51\x8f\xf1");
        item_ranges.recovery_staff=item_range_mask("ISID_\x89\xf1\x95\x9c\x8f\xf1");
        people_.clear();person_archives_.clear();person_archive_storage_.clear();
        jobs_.clear();
        items_.clear();
        skills_.clear();
        movement_costs_.clear();
        chapter_by_name_.clear();
        person_by_name_.clear();
        job_by_name_.clear();
        item_by_name_.clear();
        skill_by_name_.clear();
        person_by_id_.clear();
        job_by_id_.clear();
        item_by_id_.clear();
        skill_by_id_.clear();
        chapter_id_present_.fill(false);

        const std::size_t chapter_table = ReadRootPointer(bytes, 0x00);
        const std::size_t chapter_count = ReadU32(bytes, kPointerBase + 0x04);
        chapters_.reserve(chapter_count);
        for (std::size_t i = 0; i < chapter_count; ++i) {
            const std::size_t record = chapter_table + i * 0x1C;
            ChapterDefinition chapter{};
            chapter.cid = ReadString(bytes, ReadU32(bytes, record + 0x00));
            chapter.battlefield = ReadString(bytes, ReadU32(bytes, record + 0x04));
            chapter.battlefield_present = ReadU32(bytes, record + 0x04)!=0;
            chapter.id = bytes.at(record + 0x08);
            chapter.type = bytes.at(record + 0x09);
            for (int route = 0; route < 3; ++route) {
                chapter.next_chapter[route] = bytes.at(record + 0x0A + route);
                chapter.requirement[route] = bytes.at(record + 0x0D + route);
            }
            chapter.married_person_id = ReadU16(bytes, record + 0x10);
            chapter.offspring_seal_level = bytes.at(record + 0x12);
            chapter.offspring_seal_level_2 = bytes.at(record + 0x13);
            chapter.route_mask = bytes.at(record + 0x14);
            chapter.raw_0x15 = ReadBytes<3>(bytes, record + 0x15);
            chapter.battle_prep_screen = bytes.at(record + 0x18);
            chapter_by_name_[chapter.cid] = chapters_.size();
            chapter_by_id_[chapter.id] = chapters_.size();
            chapter_id_present_[chapter.id] = true;
            chapters_.push_back(std::move(chapter));
        }

        const std::size_t person_table = ReadRootPointer(bytes, 0x08);
        auto support_records=ParseReliance(bytes,person_table);
        const auto personality_ptr=ReadU32(bytes,0x3c),personality_count=ReadU32(bytes,0x40);
        const std::size_t personality_at=kPointerBase+personality_ptr;
        if((personality_count && !personality_ptr) || personality_at>bytes.size() ||
           personality_count>(bytes.size()-personality_at)/0x40)throw 1;
        std::vector<PersonalityDefinition> personalities;
        for(std::size_t i=0;i<personality_count;++i) {
            const auto at=personality_at+i*0x40;
            PersonalityDefinition personality{};
            personality.id=ReadU32(bytes,at);personality.attack_stance_capability=static_cast<std::int8_t>(bytes.at(at+0x3c));
            for(std::size_t lane=0;lane<8;++lane) {
                personality.boon_bases[lane]=static_cast<std::int8_t>(bytes.at(at+0x0c+lane));
                personality.bane_bases[lane]=static_cast<std::int8_t>(bytes.at(at+0x24+lane));
                personality.boon_modifiers[lane]=static_cast<std::int8_t>(bytes.at(at+0x1c+lane));
                personality.bane_modifiers[lane]=static_cast<std::int8_t>(bytes.at(at+0x34+lane));
            }
            personality.guard_stance_capabilities=ReadBytes<3>(bytes,at+0x3d);
            personalities.push_back(personality);
        }
        auto person_archive=ParsePersonArchive(bytes,person_table,next_person_archive_token_,false);
        const std::size_t person_count = person_archive.count;
        for (std::size_t i = 0; i < person_count; ++i) {
            auto person = ParsePerson(bytes, person_table + 0x10 + i * 0x98);
            person_by_name_[person.pid] = people_.size();
            person_by_id_[person.id] = people_.size();
            people_.push_back(std::move(person));
        }

        const std::size_t job_table = ReadRootPointer(bytes, 0x0C);
        const std::size_t job_count = ReadU16(bytes, job_table + 0x06);
        std::uint8_t max_movement_cost_index = 0;
        for (std::size_t i = 0; i < job_count; ++i) {
            auto job = ParseJob(bytes, job_table + 0x08 + i * 0x80);
            max_movement_cost_index = std::max(max_movement_cost_index, job.movement_cost_index);
            job_by_name_[job.jid] = jobs_.size();
            job_by_id_[job.id] = jobs_.size();
            jobs_.push_back(std::move(job));
        }

        const std::size_t item_subkind_table = ReadRootPointer(bytes, 0x34);
        for (std::size_t i = 0; i < item_subkinds_.size(); ++i) {
            const std::size_t record = item_subkind_table + i * 0x10;
            ItemSubKindDefinition sub{};
            sub.raw_category = bytes.at(record + 0x00);
            sub.weapon_exp_group = bytes.at(record + 0x01);
            sub.raw_02 = bytes.at(record + 0x02);
            sub.raw_03 = bytes.at(record + 0x03);
            sub.mikid = ReadString(bytes, ReadU32(bytes, record + 0x04));
            sub.mikid_h = ReadString(bytes, ReadU32(bytes, record + 0x08));
            sub.raw_flags = ReadU32(bytes, record + 0x0C);
            if (sub.raw_category != i) return false;
            item_subkinds_[i] = std::move(sub);
        }

        const std::size_t item_table = ReadRootPointer(bytes, 0x2C);
        const std::size_t item_count = ReadU16(bytes, item_table + 0x06);
        for (std::size_t i = 0; i < item_count; ++i) {
            auto item = ParseItem(bytes, item_table + 0x08 + i * 0x68);
            item_by_name_[item.iid] = items_.size();
            item_by_id_[item.id] = items_.size();
            items_.push_back(std::move(item));
        }

        const std::size_t skill_table = ReadRootPointer(bytes, 0x10);
        const std::size_t skill_count = ReadU32(bytes, kPointerBase + 0x18);
        for (std::size_t i = 0; i < skill_count; ++i) {
            auto skill = ParseSkill(bytes, skill_table + i * 0x20);
            skill_by_name_[skill.seid] = skills_.size();
            skill_by_id_[skill.id] = skills_.size();
            skills_.push_back(std::move(skill));
        }

        // Paragon GameData +0x38 -> three tables of ten four-byte entries.
        // Preserve raw bytes: Item::GetPower/Hit/Critical consume forge additions
        // with LDRB even though the editor exposes signed fields.
        const auto forge_pointer=ReadU32(bytes,kPointerBase+0x38);
        if(!forge_pointer)return false;
        std::array<ForgeEntryDefinition,30> forge{};
        for(std::size_t i=0;i<forge.size();++i) {
            const auto forge_bytes=ReadBytes<4>(bytes,kPointerBase+forge_pointer+i*4);
            forge[i]={forge_bytes[0],forge_bytes[1],forge_bytes[2],forge_bytes[3]};
        }
        const std::size_t rank_table = ReadRootPointer(bytes, 0x3C);
        for (std::size_t i = 0; i < weapon_rank_thresholds_.size(); ++i) {
            weapon_rank_thresholds_[i] = bytes.at(rank_table + i);
        }
        const std::size_t bonus_table = ReadRootPointer(bytes, 0x40);
        for (std::size_t category = 0; category < weapon_bonuses_.size(); ++category) {
            for (std::size_t rank = 0; rank < 6; ++rank) {
                weapon_bonuses_[category].might[rank] = static_cast<std::int8_t>(bytes.at(bonus_table + category * 12 + rank * 2));
                weapon_bonuses_[category].hit[rank] = static_cast<std::int8_t>(bytes.at(bonus_table + category * 12 + rank * 2 + 1));
            }
        }
        const std::size_t interaction_table = ReadRootPointer(bytes, 0x44);
        for (std::size_t rank = 0; rank < 6; ++rank) {
            weapon_interaction_.by_rank.might[rank] = static_cast<std::int8_t>(bytes.at(interaction_table + rank * 2));
            weapon_interaction_.by_rank.hit[rank] = static_cast<std::int8_t>(bytes.at(interaction_table + rank * 2 + 1));
        }
        for (std::size_t a = 0; a < 8; ++a) {
            for (std::size_t d = 0; d < 8; ++d) {
                weapon_interaction_.delta[a][d] = static_cast<std::int8_t>(bytes.at(interaction_table + 12 + a * 8 + d));
            }
        }

        const std::size_t movement_table = ReadRootPointer(bytes, 0x4C);
        const std::uint32_t width = ReadU32(bytes, movement_table);
        if (width == 0 || width > 64) {
            return false;
        }
        const std::uint32_t stride = (width + 3u) & ~3u;
        for (std::size_t row_index = 0; row_index <= max_movement_cost_index; ++row_index) {
            std::vector<std::int8_t> row;
            row.reserve(width);
            for (std::size_t column = 0; column < width; ++column) {
                row.push_back(static_cast<std::int8_t>(
                    bytes.at(movement_table + 4 + row_index * stride + column)));
            }
            movement_costs_.push_back(std::move(row));
        }
        personalities_=std::move(personalities);
        reliance_records_=std::move(support_records);forge_entries_=forge;forge_loaded_=true;support_tables_loaded_=true;
        const auto plan=PlanPersonArchivesExact({},PersonArchiveOperation::Initialize,person_archive);
        if(plan.status!=PersonArchiveStatus::Ok)throw 1;
        person_archives_=plan.archives;person_archive_storage_={{person_archive.token,0,people_.size(),0,reliance_records_.size(),std::make_shared<const PersonRecordReference::Instance>()}};
        ++next_person_archive_token_;RebuildPersonNames();
        person_name_unknown_hashes_.clear();person_name_metadata_known_=true;
        RecordPersonNameAuthority(bytes,person_table);RecordPersonNameAmbiguities();person_archives_known_=true;
        if(pair_bit&&*pair_bit<64)pair_prohibition_mask_=std::uint64_t(1)<<*pair_bit;
        if(protagonist_bit&&*protagonist_bit<64)protagonist_mask_=std::uint64_t(1)<<*protagonist_bit;
        if(movement_rules_known)movement_rules_=movement_rules;
        if(item_ranges_known)item_range_rules_=item_ranges;
        return true;
    } catch (...) {
        return false;
    }
}

bool DefinitionStore::LoadPersonArchive(std::span<const std::uint8_t> raw) {
    ++support_revision_;
    const bool previous_support_loaded=support_tables_loaded_;support_tables_loaded_=false;
    if(!person_archives_known_||person_archives_.empty()||!next_person_archive_token_)return false;
    std::vector<std::uint8_t> bytes;
    if(!DecompressFe14Archive(raw,bytes)||bytes.size()<0x30)return false;
    try {
        const std::size_t table=kPointerBase;
        auto archive=ParsePersonArchive(bytes,table,next_person_archive_token_,true);
        auto support_records=ParseReliance(bytes,table);
        std::vector<PersonDefinition> records;records.reserve(archive.count);
        for(std::size_t i=0;i<archive.count;++i)records.push_back(ParsePerson(bytes,table+0x10+i*0x98));
        const auto plan=PlanPersonArchivesExact(person_archives_,PersonArchiveOperation::Append,archive);
        if(plan.status!=PersonArchiveStatus::Ok)return false;
        const PersonArchiveStorage storage{archive.token,people_.size(),records.size(),reliance_records_.size(),support_records.size(),std::make_shared<const PersonRecordReference::Instance>()};
        // Preserve every archive fragment. Original numeric lookup uses the
        // first range, so later duplicate IDs must not overwrite its records.
        auto people=people_;auto reliance=reliance_records_;auto storage_next=person_archive_storage_;auto archives=plan.archives;
        people.insert(people.end(),records.begin(),records.end());
        reliance.insert(reliance.end(),support_records.begin(),support_records.end());storage_next.push_back(storage);
        NameIndex<PersonDefinition> names;for(std::size_t i=0;i<people.size();++i)names.try_emplace(people[i].pid,i);
        people_=std::move(people);reliance_records_=std::move(reliance);person_archive_storage_=std::move(storage_next);
        person_archives_=std::move(archives);person_by_name_=std::move(names);person_by_id_.clear();
        RecordPersonNameAuthority(bytes,table);RecordPersonNameAmbiguities();
        ++next_person_archive_token_;support_tables_loaded_=previous_support_loaded;
        return true;
    } catch (...) {return false;}
}

void DefinitionStore::RebuildPersonNames() {
    person_by_name_.clear();person_by_id_.clear();
    for(std::size_t i=0;i<people_.size();++i)person_by_name_.try_emplace(people_[i].pid,i);
}
void DefinitionStore::RecordPersonNameAmbiguities() {
    std::unordered_set<std::uint32_t> seen;
    for(const auto& person:people_) {
        const auto hash=HashIdentifierExact(person.pid).nameHash;
        if(person.pid.empty() || !seen.insert(hash).second)person_name_unknown_hashes_.insert(hash);
    }
}
void DefinitionStore::RecordPersonNameAuthority(const std::vector<std::uint8_t>& bytes,std::size_t person_table) {
    // ArchiveConstruct registers labels from the archive header, not the PID
    // fields themselves. Only admit a Person whose exact label points at its
    // record. Other label hashes remain outside this typed projection.
    try {
        const auto fits=[&](std::size_t at,std::size_t size){return at<=bytes.size() && size<=bytes.size()-at;};
        if(!fits(0,0x20) || bytes[0x1f] || std::equal(bytes.begin()+0x18,bytes.begin()+0x1f,"HSDArc"))throw 1;
        const std::size_t data_end=0x20+std::size_t(ReadU32(bytes,4)&~3u);
        const std::size_t relocations=ReadU32(bytes,8),labels=ReadU32(bytes,12),aux=ReadU32(bytes,16);
        if(!fits(data_end,relocations*4))throw 1;
        const auto table=data_end+relocations*4;if(!fits(table,(labels+aux)*8))throw 1;
        const auto strings=table+(labels+aux)*8;
        const auto& storage=person_archive_storage_.back();
        std::vector<unsigned> registered(storage.people_count);
        for(std::size_t i=0;i<labels;++i) {
            const std::size_t record=0x20+std::size_t(ReadU32(bytes,table+i*8)&~3u);
            const std::size_t relative=ReadU32(bytes,table+i*8+4);
            if(relative>=bytes.size()-strings || record>=data_end)throw 1;
            const auto at=strings+relative;
            const auto end=std::find(bytes.begin()+at,bytes.end(),0);if(end==bytes.end())throw 1;
            const std::string_view name(reinterpret_cast<const char*>(bytes.data()+at),std::size_t(end-(bytes.begin()+at)));
            const auto hash=HashIdentifierExact(name).nameHash;
            const auto first=person_table+0x10;
            if(record>=first && (record-first)%0x98==0 && (record-first)/0x98<storage.people_count) {
                const auto index=(record-first)/0x98;
                if(people_[storage.people_begin+index].pid==name) {
                    if(++registered[index]>1)person_name_unknown_hashes_.insert(hash);
                    continue;
                }
            }
            person_name_unknown_hashes_.insert(hash);
        }
        for(std::size_t i=0;i<registered.size();++i)if(registered[i]!=1)
            person_name_unknown_hashes_.insert(HashIdentifierExact(people_[storage.people_begin+i].pid).nameHash);
    } catch (...) {person_name_metadata_known_=false;}
}
std::optional<PersonRecordReference> DefinitionStore::ResolveUnambiguousPersonName(std::string_view name) const noexcept {
    if(!person_archives_known_ || !person_name_metadata_known_ || !name.starts_with("PID_") || name.find('\0')!=std::string_view::npos)return std::nullopt;
    const auto hash=HashIdentifierExact(name).nameHash;
    if(person_name_unknown_hashes_.contains(hash))return std::nullopt;
    for(const auto& person:people_)if(HashIdentifierExact(person.pid).nameHash==hash) {
        // Original GetIdent compares hashes, not spelling. A differently
        // spelled collision requires its real bucket and registration owner.
        if(person.pid!=name)return std::nullopt;
        return RetainPerson(person);
    }
    return PersonRecordReference{};
}
PersonRecordReference DefinitionStore::RetainPerson(const PersonDefinition& person) const noexcept {
    if(!person_archives_known_)return {};
    for(const auto& storage:person_archive_storage_)
        for(std::size_t j=0;j<storage.people_count;++j)
            if(&people_[storage.people_begin+j]==&person) {
                PersonRecordReference result;result.instance_=storage.identity;
                result.location_={storage.token,static_cast<std::uint32_t>(j)};return result;
            }
    return {};
}
const PersonDefinition* DefinitionStore::ResolvePerson(const PersonRecordReference& reference) const noexcept {
    if(!person_archives_known_ || !reference)return nullptr;
    for(const auto& storage:person_archive_storage_)
        if(storage.token==reference.location_.token && storage.identity==reference.instance_ &&
            reference.location_.record_index<storage.people_count)
            return &people_[storage.people_begin+reference.location_.record_index];
    return nullptr;
}
const PersonDefinition* DefinitionStore::PersonAtLocation(const PersonArchiveLocation& location) const {
    for(const auto& storage:person_archive_storage_)if(storage.token==location.token) {
        if(location.record_index>=storage.people_count)return nullptr;
        const auto i=storage.people_begin+location.record_index;return i<people_.size()?&people_[i]:nullptr;
    }return nullptr;
}
const PersonDefinition* DefinitionStore::GetPersonOrFirst(std::uint16_t id) const {
    if(!person_archives_known_)return nullptr;
    const auto location=FindPersonRecordExact(person_archives_,id,true);return location?PersonAtLocation(*location):nullptr;
}
std::optional<bool> DefinitionStore::IsPersonDownload(const PersonDefinition& person) const noexcept {
    const bool intrinsic=(person.bitflags[7]&0x40u)!=0;if(intrinsic)return true;
    if(!person_archives_known_)return std::nullopt;
    return PersonIsDownloadExact(person_archives_,person.id,0);
}
PersonArchivePlan DefinitionStore::ChangePersonArchives(PersonArchiveOperation op,std::string_view name) {
    if(!person_archives_known_)return {PersonArchiveStatus::InvalidState,person_archives_,{}};
    auto plan=PlanPersonArchivesExact(person_archives_,op,{},name);
    if(plan.status!=PersonArchiveStatus::Ok||plan.archives==person_archives_)return plan;
    std::vector<PersonDefinition> people;std::vector<RelianceDefinition> reliance;
    std::vector<PersonArchiveStorage> storage;
    for(const auto& archive:plan.archives) {
        const auto it=std::find_if(person_archive_storage_.begin(),person_archive_storage_.end(),[&](const auto& v){return v.token==archive.token;});
        if(it==person_archive_storage_.end()||it->people_begin+it->people_count>people_.size()||it->reliance_begin+it->reliance_count>reliance_records_.size())return {PersonArchiveStatus::InvalidState,person_archives_,{}};
        storage.push_back({archive.token,people.size(),it->people_count,reliance.size(),it->reliance_count,it->identity});
        people.insert(people.end(),people_.begin()+it->people_begin,people_.begin()+it->people_begin+it->people_count);
        reliance.insert(reliance.end(),reliance_records_.begin()+it->reliance_begin,reliance_records_.begin()+it->reliance_begin+it->reliance_count);
    }
    NameIndex<PersonDefinition> names;for(std::size_t i=0;i<people.size();++i)names.try_emplace(people[i].pid,i);
    auto archives=plan.archives;
    // Detach removes the Person list node but does not destroy the ArchiveFile
    // or its identifier entries. Keep those hashes unavailable until a new
    // core lifetime; rebuilding a string map must not manufacture absence.
    if(op==PersonArchiveOperation::Detach)for(const auto& old:person_archive_storage_)
        if(std::none_of(storage.begin(),storage.end(),[&](const auto& kept){return kept.token==old.token;}))
            for(std::size_t i=0;i<old.people_count;++i)
                person_name_unknown_hashes_.insert(HashIdentifierExact(people_[old.people_begin+i].pid).nameHash);
    people_=std::move(people);reliance_records_=std::move(reliance);person_archive_storage_=std::move(storage);person_archives_=std::move(archives);
    person_by_name_=std::move(names);person_by_id_.clear();++support_revision_;if(op==PersonArchiveOperation::Finalize)support_tables_loaded_=false;
    return plan;
}
PersonArchivePlan DefinitionStore::RemovePersonArchive(std::string_view name,bool destroy) {
    return ChangePersonArchives(destroy?PersonArchiveOperation::Free:PersonArchiveOperation::Detach,name);
}
PersonArchivePlan DefinitionStore::PrunePersonArchives() {return ChangePersonArchives(PersonArchiveOperation::Prune);}
PersonArchivePlan DefinitionStore::FinalizePersonArchives() {return ChangePersonArchives(PersonArchiveOperation::Finalize);}

bool DefinitionStore::LoadTerrainArchive(std::span<const std::uint8_t> raw) {
    std::vector<std::uint8_t> bytes;
    if (!DecompressFe14Archive(raw, bytes) || bytes.size() < 0x40) {
        return false;
    }
    try {
        const std::size_t terrain_table = kPointerBase + ReadU32(bytes, kPointerBase + 0x00);
        const std::size_t terrain_count = ReadU32(bytes, kPointerBase + 0x04);
        const std::size_t grid = kPointerBase + ReadU32(bytes, kPointerBase + 0x0C);

        TerrainMapDefinition terrain{};
        terrain.width = ReadU32(bytes, grid + 0x00);
        terrain.height = ReadU32(bytes, grid + 0x04);
        terrain.min_x = ReadU32(bytes, grid + 0x08);
        terrain.min_y = ReadU32(bytes, grid + 0x0C);
        terrain.max_x = ReadU32(bytes, grid + 0x10);
        terrain.max_y = ReadU32(bytes, grid + 0x14);
        if (grid + 0x18 + terrain.grid.size() > bytes.size()) {
            return false;
        }
        std::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(grid + 0x18),
            terrain.grid.size(),
            terrain.grid.begin());

        for (std::size_t i = 0; i < terrain_count; ++i) {
            const std::size_t record = terrain_table + i * 0x28;
            TerrainDefinition type{};
            type.tid = ReadString(bytes, ReadU32(bytes, record + 0x00));
            type.id = bytes.at(record + 0x0C);
            type.change_id_1 = bytes.at(record + 0x0D);
            type.change_id_2 = bytes.at(record + 0x0E);
            type.movement_cost_index = bytes.at(record + 0x10);
            type.defense_bonus = static_cast<std::int8_t>(bytes.at(record + 0x11));
            type.avoid_bonus = static_cast<std::int8_t>(bytes.at(record + 0x12));
            type.healing_bonus = static_cast<std::int8_t>(bytes.at(record + 0x13));
            type.flags_0x18 = ReadU32(bytes, record + 0x18);
            terrain.terrain_types.push_back(std::move(type));
        }
        terrain_ = std::move(terrain);
        terrain_loaded_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

const RelianceDefinition* DefinitionStore::FindReliance(std::uint16_t subject, std::uint16_t other) const {
    const auto* person=FindPerson(subject);
    if(!support_tables_loaded_ || !person || person->support_id<0)return nullptr;
    for(const auto& record:reliance_records_)
        if(record.owner_support_id==std::uint16_t(person->support_id) && record.character_id==other)return &record;
    return nullptr;
}

const ChapterDefinition* DefinitionStore::FindChapter(std::string_view cid) const {
    return FindByName(chapters_, chapter_by_name_, cid);
}
const ChapterDefinition* DefinitionStore::FindChapter(std::uint8_t id) const {
    return chapter_id_present_[id] ? &chapters_[chapter_by_id_[id]] : nullptr;
}
const PersonDefinition* DefinitionStore::FindPerson(std::string_view pid) const {
    if(!person_archives_known_)return nullptr;
    return FindByName(people_, person_by_name_, pid);
}
const PersonDefinition* DefinitionStore::FindPerson(std::uint16_t id) const {
    if(!person_archives_known_)return nullptr;
    const auto location=FindPersonRecordExact(person_archives_,id);return location?PersonAtLocation(*location):nullptr;
}
const JobDefinition* DefinitionStore::FindJob(std::string_view jid) const {
    return FindByName(jobs_, job_by_name_, jid);
}
const JobDefinition* DefinitionStore::FindJob(std::uint16_t id) const {
    return FindById(jobs_, job_by_id_, id);
}
const JobDefinition* DefinitionStore::ResolveDisposJob(
    const PersonDefinition& person, std::string_view authored_jid) const {
    if (!authored_jid.empty()) {
        return FindJob(authored_jid);
    }
    return person.class_1 != 0 ? FindJob(person.class_1) : nullptr;
}
const ItemDefinition* DefinitionStore::FindItem(std::string_view iid) const {
    return FindByName(items_, item_by_name_, iid);
}
const ItemDefinition* DefinitionStore::FindItem(std::uint16_t id) const {
    return FindById(items_, item_by_id_, id);
}
const SkillDefinition* DefinitionStore::FindSkill(std::string_view seid) const {
    return FindByName(skills_, skill_by_name_, seid);
}
const SkillDefinition* DefinitionStore::FindSkill(std::uint16_t id) const {
    return FindById(skills_, skill_by_id_, id);
}

const ItemSubKindDefinition* DefinitionStore::FindItemSubKind(std::uint8_t raw_category) const {
    return raw_category < item_subkinds_.size() ? &item_subkinds_[raw_category] : nullptr;
}
std::uint8_t DefinitionStore::WeaponExpGroupForItem(const ItemDefinition& item) const noexcept {
    return item.weapon_category < item_subkinds_.size() ? item_subkinds_[item.weapon_category].weapon_exp_group : 0xFFu;
}
bool DefinitionStore::IsMagicItem(const ItemDefinition& item) const noexcept {
    const auto group = WeaponExpGroupForItem(item);
    return IsMagicItemGroupExact(group,(item.bitflags[0] & 0x02u) != 0);
}
std::uint16_t DefinitionStore::BaseJobCategoryMask(const JobDefinition& job) const noexcept {
    return static_cast<std::uint16_t>(job.special_flags[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(job.special_flags[1]) << 8);
}

int DefinitionStore::WeaponRankIndex(std::uint8_t exp) const noexcept {
    for (std::size_t i = 0; i < weapon_rank_thresholds_.size(); ++i) {
        if (exp >= weapon_rank_thresholds_[i]) return static_cast<int>(i);
    }
    return -1;
}
std::int8_t DefinitionStore::WeaponRankMightBonus(std::uint8_t category, std::uint8_t exp) const noexcept {
    const int rank = WeaponRankIndex(exp);
    return category < weapon_bonuses_.size() && rank >= 0 ? weapon_bonuses_[category].might[static_cast<std::size_t>(rank)] : 0;
}
std::int8_t DefinitionStore::WeaponRankHitBonus(std::uint8_t category, std::uint8_t exp) const noexcept {
    const int rank = WeaponRankIndex(exp);
    return category < weapon_bonuses_.size() && rank >= 0 ? weapon_bonuses_[category].hit[static_cast<std::size_t>(rank)] : 0;
}
std::int8_t DefinitionStore::WeaponInteractionDelta(std::uint8_t attacker, std::uint8_t defender) const noexcept {
    return attacker < 8 && defender < 8 ? weapon_interaction_.delta[attacker][defender] : 0;
}
std::int8_t DefinitionStore::WeaponInteractionMight(std::uint8_t rank) const noexcept {
    return rank < 6 ? weapon_interaction_.by_rank.might[rank] : 0;
}
std::int8_t DefinitionStore::WeaponInteractionHit(std::uint8_t rank) const noexcept {
    return rank < 6 ? weapon_interaction_.by_rank.hit[rank] : 0;
}

const TerrainDefinition* DefinitionStore::TerrainAt(std::uint32_t x, std::uint32_t y) const {
    if (!terrain_loaded_ || x >= 32 || y >= 32) {
        return nullptr;
    }
    const std::uint8_t index = terrain_.grid[x | (y << 5)];
    return index < terrain_.terrain_types.size() ? &terrain_.terrain_types[index] : nullptr;
}

} // namespace fates::runtime::native
