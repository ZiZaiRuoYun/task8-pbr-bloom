#pragma once
#include <glad/glad.h>

// ˵����Ϊ���ݾɹ��ߣ����� gPosition / gNormal / gAlbedoSpec��
// ͬʱ���� PBR ����Ķ���������gAlbedo / gNormalRough / gMetalAO / gEmissive��
class GBuffer {
public:
    // ��ʼ�� / �ߴ���
    bool init(int width, int height);
    void resize(int width, int height);

    // �󶨣����ν׶�д�� / ���ս׶ζ�ȡ��������
    void bindForGeometryPass(); 
    void bindForReading();

    // �ɸ��������� Blinn�CPhong��
    GLuint getPosition() const { return gPosition_; }
    GLuint getNormal()   const { return gNormal_; }
    GLuint getAlbedoSpec() const { return gAlbedoSpec_; }

    // ֡����/FBO ��ʵ�ú���
    GLuint fbo() const { return fbo_; }
    void blitDepthToDefault(int dstW, int dstH);

    // NEW: PBR ����
    // Albedo: ����ɫ�������� legacy gAlbedoSpec��
    GLuint getAlbedo()        const { return gAlbedo_; }
    // NormalRough: xyz = ���ߣ����Դ洢���� [-1,1] ӳ���� shader ���𣩣�a = roughness
    GLuint getNormalRough()   const { return gNormalRough_; }
    // MetalAO: r = metallic, g = ao
    GLuint getMetalAO()       const { return gMetalAO_; }
    // Emissive: �������ѡʹ�ã�
    GLuint getEmissive()      const { return gEmissive_; }

private:
    void destroy();
    void allocTextures(int w, int h);   // �ڲ�����/���·������и���

    // FBO / ���
    GLuint fbo_ = 0, rboDepth_ = 0;

    // �ɸ���
    GLuint gPosition_ = 0; // RGBA16F
    GLuint gNormal_ = 0; // RGBA16F
    GLuint gAlbedoSpec_ = 0; // RGBA8��a ͨ������ spec��

    // NEW: PBR ����
    GLuint gAlbedo_ = 0; // SRGB8_ALPHA8 �� RGBA8��д�������ԡ�����׶�ͳһ�� gamma��
    GLuint gNormalRough_ = 0; // RGBA16F��xyz=normal, a=roughness��
    GLuint gMetalAO_ = 0; // RG8 / RG16F��r=metallic, g=ao��
    GLuint gEmissive_ = 0; // RGB16F����ѡ��

    int w_ = 0, h_ = 0;
};

