#include "post/effects/ToneMapping.h"
#include "Shader.h"

ToneMapEffect::ToneMapEffect() {
	sh_ = std::make_unique<Shader>();
	sh_->Load("shaders/blit.vert", "shaders/pp/tonemap.frag"); // 复用已有 VS
}
void ToneMapEffect::setParams(float e, int m, bool g) { exposure_ = e; mode_ = m; gamma_ = g; }
//void ToneMapEffect::process(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO) {
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glViewport(0, 0, screenW, screenH);
//    glDisable(GL_DEPTH_TEST);
//
//    sh_->Use();
//    sh_->SetInt("uHDR", 0);
//    sh_->SetInt("uGMask", 8);                // 用 8 号单元绑定几何遮罩
//    sh_->SetFloat("uExposure", exposure_);
//    sh_->SetInt("uMode", mode_);             // 0=Reinhard, 1=ACES
//    sh_->SetInt("uDoGamma", gamma_ ? 1 : 0);
//    sh_->SetInt("uBloom", 9);
//
//    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, hdrTex);
//    glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, gMaskTex);
//
//    glBindVertexArray(fsVAO);                // 绑定全屏三角 VAO
//    glDrawArrays(GL_TRIANGLES, 0, 3);
//    glBindVertexArray(0);
//}

void ToneMapEffect::process(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO,
    bool useBloom, GLuint bloomTex, float bloomStrength)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glDisable(GL_DEPTH_TEST);

    sh_->Use();
    sh_->SetInt("uHDR", 0);
    sh_->SetInt("uGMask", 8);
    sh_->SetFloat("uExposure", exposure_);
    sh_->SetInt("uMode", mode_);
    sh_->SetInt("uDoGamma", gamma_ ? 1 : 0);

    sh_->SetInt("uUseBloom", useBloom ? 1 : 0);
    sh_->SetInt("uBloom", 9);
    sh_->SetFloat("uBloomStrength", bloomStrength);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, hdrTex);
    glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, gMaskTex);
    if (useBloom) {
        glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_2D, bloomTex);
    }

    glBindVertexArray(fsVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

