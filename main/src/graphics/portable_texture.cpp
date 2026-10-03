#include "fates/graphics/portable_texture.hpp"
#include <algorithm>
#include <array>

namespace fates::graphics::portable {
namespace {
using Color=std::array<std::uint8_t,4>;
constexpr int Modifiers[8][4]={{2,8,-2,-8},{5,17,-5,-17},{9,29,-9,-29},{13,42,-13,-42},{18,60,-18,-60},{24,80,-24,-80},{33,106,-33,-106},{47,183,-47,-183}};
std::uint64_t Little64(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint64_t value=0;for(unsigned i=0;i<8;++i)value|=std::uint64_t(bytes[at+i])<<(8*i);return value;
}
int Signed3(unsigned value){return int(value&7u)-((value&4u)?8:0);}
int Expand5(unsigned value){return int((value<<3)|(value>>2));}
std::uint8_t Clamp(int value){return static_cast<std::uint8_t>(std::clamp(value,0,255));}
// GraphicsLab5/19 ETC block decoder; native PICA color block byte swap is
// represented by this little-endian word. Alpha nibbles are not byte-swapped.
std::array<Color,16> EtcBlock(std::uint64_t bits) {
    const auto low=static_cast<std::uint32_t>(bits),high=static_cast<std::uint32_t>(bits>>32);
    const bool differential=(high&2u)!=0,flip=(high&1u)!=0;
    const unsigned table1=(high>>5)&7u,table2=(high>>2)&7u;
    std::array<int,3> first,second;
    if(differential) {
        const unsigned red=(high>>27)&31u,green=(high>>19)&31u,blue=(high>>11)&31u;
        first={Expand5(red),Expand5(green),Expand5(blue)};
        second={Expand5(static_cast<unsigned>(int(red)+Signed3(high>>24))&31u),Expand5(static_cast<unsigned>(int(green)+Signed3(high>>16))&31u),Expand5(static_cast<unsigned>(int(blue)+Signed3(high>>8))&31u)};
    } else {
        first={int((high>>28)&15u)*17,int((high>>20)&15u)*17,int((high>>12)&15u)*17};
        second={int((high>>24)&15u)*17,int((high>>16)&15u)*17,int((high>>8)&15u)*17};
    }
    std::array<Color,16> output;
    for(unsigned x=0;x<4;++x)for(unsigned y=0;y<4;++y) {
        const unsigned i=x*4+y,index=(((low>>(i+16))&1u)<<1)|((low>>i)&1u);
        const bool latter=flip?y>=2:x>=2;const auto& color=latter?second:first;
        const int modifier=Modifiers[latter?table2:table1][index];
        output[y*4+x]={Clamp(color[0]+modifier),Clamp(color[1]+modifier),Clamp(color[2]+modifier),255};
    }
    return output;
}
unsigned Morton8(unsigned x,unsigned y){return (x&1u)|((y&1u)<<1)|((x&2u)<<1)|((y&2u)<<2)|((x&4u)<<2)|((y&4u)<<3);}
bool Fail(std::string& error,const char* cause){error=cause;return false;}
}
bool DecodeTextureBase(std::span<const std::uint8_t> bytes,unsigned format,std::uint32_t width,std::uint32_t height,TextureImage& output,std::string& error) {
    if(width<8||height<8||width>8192||height>8192||(width&(width-1))||(height&(height-1)))return Fail(error,"Unsupported texture extent");
    unsigned bytes_per_pixel;
    switch(format){case 0:bytes_per_pixel=4;break;case 1:bytes_per_pixel=3;break;case 3:case 4:case 5:case 6:bytes_per_pixel=2;break;case 7:case 8:case 9:case 10:case 11:case 12:case 13:bytes_per_pixel=1;break;default:return Fail(error,"Unsupported texture pixel format");}
    const std::size_t pixels=std::size_t(width)*height;
    const bool nibbles=format==10 || format==11;
    const auto required=(format==12 || nibbles)?pixels/2:pixels*bytes_per_pixel;
    if(bytes.size()<required)return Fail(error,"Truncated texture base mip");
    TextureImage next{width,height,std::vector<std::uint8_t>(pixels*4)};
    const auto store=[&](unsigned x,unsigned y,const Color& color) {
        const std::size_t offset=(std::size_t(height-1-y)*width+x)*4;
        std::copy(color.begin(),color.end(),next.rgba.begin()+offset);
    };
    if(format==12||format==13) {
        std::size_t at=0;
        for(unsigned ty=0;ty<height;ty+=8)for(unsigned tx=0;tx<width;tx+=8)
            for(unsigned block=0;block<4;++block) {
                const bool alpha=format==13;const auto a=alpha?Little64(bytes,at):0;
                const auto colors=EtcBlock(Little64(bytes,at+(alpha?8:0)));at+=alpha?16:8;
                const auto bx=(block&1u)*4,by=(block>>1)*4;
                for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x) {
                    auto color=colors[y*4+x];if(alpha)color[3]=static_cast<std::uint8_t>(((a>>((x*4+y)*4))&15u)*17);
                    store(tx+bx+x,ty+by+y,color);
                }
            }
    } else {
        // Supplied BasicGraphics Page_55588693, sections7.3/7.10: native byte
        // swapping for unsigned-byte multicomponent pixels, V flip and8x8 Morton
        // tiles. Packed16-bit RGB565/RGBA4 keep standard packed bit significance.
        for(unsigned ty=0;ty<height;ty+=8)for(unsigned tx=0;tx<width;tx+=8)
            for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
                const std::size_t tile=(std::size_t(ty/8)*(width/8)+tx/8)*64;
                const auto pixel=tile+Morton8(x,y);
                const auto at=nibbles?pixel/2:pixel*bytes_per_pixel;Color color{};
                switch(format) {
                case 0:color={bytes[at+3],bytes[at+2],bytes[at+1],bytes[at]};break;
                case 1:color={bytes[at+2],bytes[at+1],bytes[at],255};break;
                case 6:color={bytes[at+1],bytes[at],0,255};break;
                case 8:color={255,255,255,bytes[at]};break;
                case 5:color={bytes[at+1],bytes[at+1],bytes[at+1],bytes[at]};break;
                case 7:color={bytes[at],bytes[at],bytes[at],255};break;
                case 9:{const auto l=static_cast<std::uint8_t>((bytes[at]>>4u)*17u),a=static_cast<std::uint8_t>((bytes[at]&15u)*17u);color={l,l,l,a};break;}
                case 10:case 11:{const auto value=static_cast<std::uint8_t>(((bytes[at]>>((pixel&1u)*4u))&15u)*17u);
                    color=format==10?Color{value,value,value,255}:Color{255,255,255,value};break;}
                case 3:{const auto word=unsigned(bytes[at])|(unsigned(bytes[at+1])<<8);const auto g=(word>>5)&63u;color={static_cast<std::uint8_t>(Expand5(word>>11)),static_cast<std::uint8_t>((g<<2)|(g>>4)),static_cast<std::uint8_t>(Expand5(word&31u)),255};break;}
                case 4:{const auto word=unsigned(bytes[at])|(unsigned(bytes[at+1])<<8);color={static_cast<std::uint8_t>(((word>>12)&15u)*17),static_cast<std::uint8_t>(((word>>8)&15u)*17),static_cast<std::uint8_t>(((word>>4)&15u)*17),static_cast<std::uint8_t>((word&15u)*17)};break;}
                default:return Fail(error,"Unsupported texture pixel format");
                }
                store(tx+x,ty+y,color);
            }
    }
    output=std::move(next);error.clear();return true;
}
}
