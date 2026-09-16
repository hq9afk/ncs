#ifndef SHADERS_H
#define SHADERS_H

#include "errors.h"
#include <epoxy/gl.h>

#include <iostream>
#include <map>
#include <string>

enum ShaderTypes {
    VERTEX,
    FRAGMENT
};

class ShaderCompilationArgs {
public:
    unsigned int* glProgram = NULL;
    int windowWidth = 0, windowHeight = 0;
    std::map<std::string, int>* uniformLocations = NULL;
};

class VertexShaderCompilationArgs : public ShaderCompilationArgs {
public:
    VertexShaderCompilationArgs(int windowWidth, int windowHeight, unsigned int* glProgram,
        std::map<std::string, int>* uniformLocations)
    {
        this->windowWidth = windowWidth;
        this->windowHeight = windowHeight;
        this->glProgram = glProgram;
        this->uniformLocations = uniformLocations;
    }
};

class FragmentShaderCompilationArgs : public ShaderCompilationArgs {

public:
    FragmentShaderCompilationArgs(int windowWidth, int windowHeight, unsigned int* glProgram,
        std::map<std::string, int>* uniformLocations)
    {
        this->windowWidth = windowWidth;
        this->windowHeight = windowHeight;
        this->glProgram = glProgram;
        this->uniformLocations = uniformLocations;
    }
};

class Shader {
protected:
    virtual void compileShaderSource(

        ShaderCompilationArgs* args) = 0;

    void checkCompileErrors(std::string type);

public:
    virtual ~Shader() = default;

    int windowWidth = 0, windowHeight = 0;
    unsigned int *glProgram = NULL, shaderObject = 0;
    std::string shaderSource = "";
};

class VertexShader : public Shader {
private:
    const std::string defaultVertexShaderSource = "#version 100\n"
                                                  "attribute vec3 aPos;\n"
                                                  "void main()\n"
                                                  "{\n"
                                                  "    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
                                                  "}";

protected:
    void compileShaderSource(

        ShaderCompilationArgs* args);

public:
    unsigned int vertexBufferObject = 0;

    const GLfloat buf[18] = { -1, -1, 0, 1, -1, 0, -1, 1, 0,
        1, 1, 0, 1, -1, 0, -1, 1, 0 };

    VertexShader(std::string shaderSource,

        ShaderCompilationArgs* args);
    void draw(unsigned int* texture = NULL);

    // Draws `count` GL_POINTS sourced from `pointVbo` (2 floats per vertex, the
    // pixel each point occupies) with additive blending, instead of the
    // full-screen quad. Used for the particle-accumulation stage, which on
    // ES 3.2 scattered into an atomic image and here scatters via the blend
    // stage instead.
    void drawPoints(unsigned int pointVbo, int count);

    ~VertexShader();
};

class FragmentShader : public Shader {
protected:
    void compileShaderSource(

        ShaderCompilationArgs* args);

public:
    unsigned int frameBufferObject = 0, outputTexture = 0;

    void bind2DTextureToFrameBuffer(char* errorContext)
    {

        glGenFramebuffers(1, &frameBufferObject);
        glGenTextures(1, &outputTexture);
        glBindTexture(GL_TEXTURE_2D, outputTexture);

        // ES 2.0 glTexImage2D takes an unsized internal format (must match `format`).
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // ES 3.1 has no border clamp in core; use the EXT when present so the
        // out-of-range glow samples match desktop, else fall back to edge clamp.
        if (epoxy_has_gl_extension("GL_EXT_texture_border_clamp")) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        // ES 2.0 has a single GL_FRAMEBUFFER target (no separate read/draw
        // targets -- that split is ES 3.0+).
        glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            outputTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Errors::throwError("an error occured while binding texture to framebuffer", std::string(errorContext), std::string("In"));
            exit(0);
        }
    }

    FragmentShader()
    {
    }

    FragmentShader(std::string shaderSource,

        ShaderCompilationArgs* args);

    void updateUniforms(int windowWidth, int windowHeight, int left, int right, float ticks, unsigned int audioLSize, unsigned int audioLTexture, unsigned int audioRSize, unsigned int audioRTexture,
        std::map<std::string, int> uniformLocations);
    ~FragmentShader();
};

#endif