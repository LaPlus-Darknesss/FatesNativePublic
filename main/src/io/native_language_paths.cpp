#include "fates/io/native_language_paths.hpp"
#include <algorithm>

namespace fates::io::native {
namespace {
bool Valid(std::string_view value) {return value.find('\0')==std::string_view::npos;}
template<std::size_t N>
void Append(std::array<char,N>& buffer,std::size_t& used,std::string_view text,std::size_t limit) {
    const auto count=std::min({text.size(),limit,N-1-used});
    std::copy_n(text.begin(),count,buffer.begin()+used);
    used+=count;buffer[used]=0;
}
LanguagePathResult PrepareFolder(std::string_view path,const NativeLanguageState& state,
    const std::function<FileSourceStatus(std::string_view)>& exists,std::array<char,0x80>& buffer) {
    using S=LanguagePathStatus;
    if(!Valid(path))return {S::InvalidPath};
    if(!state.language)return {S::UnknownLanguage};
    std::string_view folder;
    switch(*state.language) {
    case 0:folder="@J/";break;
    case 2:folder="@F/";break;
    case 3:folder="@G/";break;
    case 4:folder="@I/";break;
    case 5:folder="@S/";break;
    default:
        if(!state.region)return {S::UnknownRegion};
        folder=*state.region==2?"@U/":"@E/";break;
    }
    const auto slash=path.find_last_of('/');
    const auto filename=slash==std::string_view::npos?0:slash+1;
    std::size_t used=0;
    Append(buffer,used,path.substr(0,filename),filename);
    Append(buffer,used,folder,3);
    Append(buffer,used,path.substr(filename),99);
    const std::string localized(buffer.data(),used);
    if(!exists)return {S::SourceUnavailable};
    const auto status=exists(localized);
    if(status==FileSourceStatus::Unavailable)return {S::SourceUnavailable};
    return {S::Ready,status==FileSourceStatus::Ready?localized:std::string(path),
        status==FileSourceStatus::Ready};
}
}
LanguagePathResult NativeLanguagePaths::Folder(std::string_view path,const NativeLanguageState& state) {
    auto buffer=language_buffer_;
    auto result=PrepareFolder(path,state,exists_,buffer);
    if(result.status==LanguagePathStatus::Ready)language_buffer_=buffer;
    return result;
}
LanguagePathResult NativeLanguagePaths::FolderForTexture(std::string_view path,const NativeLanguageState& state) {
    if(!Valid(path))return {LanguagePathStatus::InvalidPath};
    if(!state.region)return {LanguagePathStatus::UnknownRegion};
    if(*state.region!=2)return {LanguagePathStatus::Ready,std::string(path),false};
    // Both original functions use the SAME persistent128-byte buffer. The
    // region gate precedes the language read even for its explicit language arms.
    return Folder(path,state);
}
LanguagePathResult NativeLanguagePaths::CreateMessageName(std::optional<std::string_view> route,
    std::string_view archive,const NativeLanguageState& state) {
    if(!Valid(archive) || (route && !Valid(*route)))return {LanguagePathStatus::InvalidPath};
    auto message=message_buffer_;auto language=language_buffer_;std::size_t used=0;
    Append(message,used,"m/",2);
    if(route) {Append(message,used,*route,route->size());Append(message,used,"/",1);}
    Append(message,used,archive,archive.size());
    Append(message,used,".bin.lz",7);
    auto result=PrepareFolder(std::string_view(message.data(),used),state,exists_,language);
    if(result.status!=LanguagePathStatus::Ready)return result;
    // Original sut::strncpy uses80 total bytes and keeps untouched buffer tail.
    used=0;Append(message,used,result.path,message.size()-1);
    result.path.assign(message.data(),used);
    message_buffer_=message;language_buffer_=language;
    return result;
}
LanguagePathResult NativeLanguagePaths::CreateCommonMessageName(std::string_view archive) {
    if(!Valid(archive))return {LanguagePathStatus::InvalidPath};
    auto message=message_buffer_;std::size_t used=0;
    Append(message,used,"m/common/",9);Append(message,used,archive,archive.size());Append(message,used,".bin.lz",7);
    message_buffer_=message;return {LanguagePathStatus::Ready,std::string(message.data(),used),false};
}
}
