#pragma once
#include <glad/glad.h>
#include <memory>
class Shader;

class ToneMapEffect {
public:
	ToneMapEffect();
	void setParams(float exposure, int mode, bool gamma);
	//void process(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO);
	void ToneMapEffect::process(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO,
		bool useBloom, GLuint bloomTex, float bloomStrength);
private:
	float exposure_ = 1.0f; int mode_ = 1; bool gamma_ = true;
	std::unique_ptr<Shader> sh_;
};
