#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "MglMath.h"

class Shader {
public:
    unsigned int ID;

    Shader() : ID(0) {}

    Shader(const char* vertPath, const char* fragPath) {
        std::string vertCode, fragCode;
        std::ifstream vFile(vertPath), fFile(fragPath);
        if (!vFile.is_open() || !fFile.is_open()) {
            std::cerr << "ERROR: Shader file not found: " << vertPath << " / " << fragPath << std::endl;
            ID = 0;
            return;
        }
        std::stringstream vStream, fStream;
        vStream << vFile.rdbuf();
        fStream << fFile.rdbuf();
        vertCode = vStream.str();
        fragCode = fStream.str();
        const char* vSrc = vertCode.c_str();
        const char* fSrc = fragCode.c_str();

        unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vSrc, nullptr);
        glCompileShader(vert);
        checkCompile(vert, "VERTEX");

        unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fSrc, nullptr);
        glCompileShader(frag);
        checkCompile(frag, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        checkLink();

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(ID); }

    // ── Uniform setters ───────────────────────────────
    void setBool(const std::string& name, bool v)      const { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)v); }
    void setInt(const std::string& name, int v)        const { glUniform1i(glGetUniformLocation(ID, name.c_str()), v); }
    void setFloat(const std::string& name, float v)    const { glUniform1f(glGetUniformLocation(ID, name.c_str()), v); }
    void setVec3(const std::string& name, const glm::vec3& v) const { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v)); }
    void setVec3(const std::string& name, float x, float y, float z) const { glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); }
    void setVec4(const std::string& name, const glm::vec4& v) const { glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v)); }
    void setMat4(const std::string& name, const glm::mat4& m) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_TRUE, glm::value_ptr(m)); }

private:
    void checkCompile(unsigned int shader, const char* type) {
        int ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[512]; glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "SHADER COMPILE ERROR (" << type << "): " << info << std::endl;
        }
    }
    void checkLink() {
        int ok; glGetProgramiv(ID, GL_LINK_STATUS, &ok);
        if (!ok) {
            char info[512]; glGetProgramInfoLog(ID, 512, nullptr, info);
            std::cerr << "SHADER LINK ERROR: " << info << std::endl;
        }
    }
};
