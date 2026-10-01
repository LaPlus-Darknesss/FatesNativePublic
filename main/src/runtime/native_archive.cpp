#include "fates/runtime/native_archive.hpp"
#include <cstddef>
#include <algorithm>

namespace fates::runtime::native {
ArchiveLabelResult FindArchiveLabel(std::span<const std::uint8_t> b,std::string_view label) {
    auto fits=[&](std::size_t at,std::size_t count){return at<=b.size()&&count<=b.size()-at;};
    auto word=[&](std::size_t at){return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|(std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);};
    if(!fits(0,0x20))return {};
    // Constructed buffers contain runtime addresses, not payload-relative data.
    if(b[0x1f]||std::equal(b.begin()+0x18,b.begin()+0x1f,"HSDArc"))return {};
    const std::size_t data_end=std::size_t(word(4)&~3u)+0x20,relocations=word(8),labels=word(12),aux=word(16);
    if(!fits(data_end,relocations*4))return {};
    const auto table=data_end+relocations*4;
    if(!fits(table,(labels+aux)*8))return {};
    const auto strings=table+(labels+aux)*8;std::optional<std::size_t> result;
    for(std::size_t i=0;i<labels;++i) {
        const auto record=std::size_t(word(table+8*i)&~3u)+0x20;
        const auto relative=std::size_t(word(table+8*i+4));
        if(relative>b.size()-strings)return {};
        const auto at=strings+relative;if(!fits(at,1))return {};
        auto end=at;while(end<b.size()&&b[end])++end;if(end==b.size())return {};
        if(std::string_view(reinterpret_cast<const char*>(b.data()+at),end-at)!=label)continue;
        if(result||record<0x20||record>data_end||data_end-record<4)return {};
        result=record;
    }return result?ArchiveLabelResult{ArchiveLabelStatus::Found,*result}:ArchiveLabelResult{ArchiveLabelStatus::Missing,0};
}
std::optional<std::size_t> ReadArchiveLabeledOffset(std::span<const std::uint8_t> b,std::string_view label) {
    const auto result=FindArchiveLabel(b,label);
    return result.status==ArchiveLabelStatus::Found?std::optional<std::size_t>(result.offset):std::nullopt;
}
std::optional<std::uint32_t> ReadArchiveLabeledWord(std::span<const std::uint8_t> b,std::string_view label) {
    const auto at=ReadArchiveLabeledOffset(b,label);if(!at)return std::nullopt;
    return std::uint32_t(b[*at])|(std::uint32_t(b[*at+1])<<8)|(std::uint32_t(b[*at+2])<<16)|(std::uint32_t(b[*at+3])<<24);
}

bool DecompressFe14Archive(
    std::span<const std::uint8_t> input,
    std::vector<std::uint8_t>& output) {
    if (input.size() < 8 || input[0] != 0x13 || input[4] != 0x11) {
        return false;
    }

    const std::uint8_t* data = input.data() + 4;
    const std::size_t input_size = input.size() - 4;
    const std::size_t output_size =
        static_cast<std::size_t>(data[1]) |
        (static_cast<std::size_t>(data[2]) << 8) |
        (static_cast<std::size_t>(data[3]) << 16);

    std::size_t input_pos = 4;
    output.clear();
    output.reserve(output_size);

    while (output.size() < output_size) {
        if (input_pos >= input_size) {
            return false;
        }
        const std::uint8_t flags = data[input_pos++];
        for (int bit = 0; bit < 8 && output.size() < output_size; ++bit) {
            if ((flags & (0x80u >> bit)) == 0) {
                if (input_pos >= input_size) {
                    return false;
                }
                output.push_back(data[input_pos++]);
                continue;
            }

            if (input_pos + 1 >= input_size) {
                return false;
            }
            const std::uint8_t b1 = data[input_pos++];
            const std::uint8_t b2 = data[input_pos++];
            const std::uint8_t high = b1 >> 4;
            std::size_t length = 0;
            std::size_t displacement = 0;

            if (high == 0) {
                if (input_pos >= input_size) {
                    return false;
                }
                const std::uint8_t b3 = data[input_pos++];
                length = ((b1 & 0x0F) << 4 | (b2 >> 4)) + 0x11;
                displacement = ((b2 & 0x0F) << 8 | b3) + 1;
            } else if (high == 1) {
                if (input_pos + 1 >= input_size) {
                    return false;
                }
                const std::uint8_t b3 = data[input_pos++];
                const std::uint8_t b4 = data[input_pos++];
                length = ((b1 & 0x0F) << 12 |
                          (static_cast<std::size_t>(b2) << 4) |
                          (b3 >> 4)) + 0x111;
                displacement = ((b3 & 0x0F) << 8 | b4) + 1;
            } else {
                length = high + 1;
                displacement = ((b1 & 0x0F) << 8 | b2) + 1;
            }

            if (displacement == 0 || displacement > output.size()) {
                return false;
            }
            for (std::size_t i = 0; i < length && output.size() < output_size; ++i) {
                output.push_back(output[output.size() - displacement]);
            }
        }
    }
    return output.size() == output_size;
}
}
