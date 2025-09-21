#pragma once
#include <glad/glad.h>
#include <memory>
#include "Shader.h"
#include "post/effects/ToneMapping.h"
#include "post/effects/Bloom.h"

class BloomEffect;
class ToneMapEffect;
class Shader;

class PostProcessor {
public:
	PostProcessor(int w, int h);
	~PostProcessor();
	void resize(int w, int h);
	void run(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO);

	// Tonemap 参数
	void setToneMapParams(float exposure, int mode, bool gamma) {
		tmExposure = exposure;
		tmMode = mode;
		tmGamma = gamma;
	}
	// Bloom 参数/开关
	void setBloom(bool enable, float threshold, float knee, int iters, float strength, int downscale);

private:
	int w_ = 0, h_ = 0;
	std::unique_ptr<ToneMapEffect> tonemap_;
	std::unique_ptr<BloomEffect>   bloom_;

	float tmExposure = 1.0f; int tmMode = 1; bool tmGamma = true;
	bool  bloomEnable = true; float bloomStrength = 0.8f;
};

