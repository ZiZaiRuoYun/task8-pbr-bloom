#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    void Load(const std::string& vertPath, const std::string& fragPath);
    void Use() const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    unsigned int Get() const { return ID; }  // ·µ»Ø program handle
    void SetVec2(const std::string& name, const glm::vec2& value) const;


private:
    unsigned int ID = 0;
    unsigned int CompileShader(unsigned int type, const std::string& source);
    std::string LoadFromFile(const std::string& path);
};
