#pragma once
class ITexture; enum class PicaDataTextureFormat:int;
class PostEffectContext {
public:
    PostEffectContext(); virtual ~PostEffectContext();
    virtual void OnBegin(){} virtual void OnEnd(){} virtual void Draw(){}
    void DefaultSetup(); void TextureAssign(int,const ITexture*,unsigned int); void TextureAssign(int,unsigned int,unsigned int,unsigned int,PicaDataTextureFormat); void InitBeforeDraw(int,int); void TevSetGaussPass1(int,int,int); void TevSetGaussPass2(int,int,int); void DrawQuads(int,int);
private: void* buffers_[2]{};
};
