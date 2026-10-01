#include "fates/progression/progression_support_depth.hpp"

namespace fates::progression {

std::array<std::uint32_t,4> ClearIdentifierWords() {
    return {0u,0u,0u,0u};
}
IdentifierSerializedV0 DescribeIdentifierSerializationV0(const std::array<std::uint32_t,4>& words) {
    IdentifierSerializedV0 out{};
    out.first_u64_words = {words[0],words[1]};
    out.second_u64_words = {words[2],words[3]};
    return out;
}
std::vector<std::uint8_t> SerializeUnitEditV4(const std::array<std::uint8_t,0x30>& edit) {
    std::vector<std::uint8_t> out;
    out.reserve(44);
    out.push_back(4u);
    out.insert(out.end(),edit.begin(),edit.begin()+0x1a);
    out.push_back(edit[0x1a]);
    out.push_back(edit[0x1b]);
    out.push_back(edit[0x1e]);
    out.push_back(edit[0x1f]);
    out.push_back(edit[0x20]);
    out.push_back(edit[0x21]);
    out.push_back(edit[0x22]);
    out.push_back(edit[0x23]);
    out.insert(out.end(),edit.begin()+0x24,edit.begin()+0x28);
    out.push_back(edit[0x28]);
    out.push_back(edit[0x29]);
    out.push_back(edit[0x2a]);
    out.push_back(edit[0x2b]);
    out.push_back(edit[0x2c]);
    return out;
}

bool BuddyInheritanceCandidateEligible(bool candidate_is_player,bool candidate_is_download,
                                       bool same_sex,bool is_family,bool is_romance,
                                       int reliance_level) {
    return !candidate_is_player && !candidate_is_download && same_sex &&
           !is_family && !is_romance && reliance_level > 2;
}
int AbsoluteNameWindowTextureWidth(std::int16_t signed_width) {
    const int w=static_cast<int>(signed_width);
    return w<0?-w:w;
}

} // namespace fates::progression
