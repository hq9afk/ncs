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

    // `highPrecision` is for the particle-accumulation stage's target: it sums
    // many overlapping additive splats (see ShaderProgram::render()), and an
    // 8-bit UNORM target clamps that running sum to 1.0 almost immediately,
    // flattening the density gradient ncs-2.frag's actualDepth depends on
    // (that gradient is what glow-1.frag's bloom actually has to work with,
    // which is why a saturated accumulator reads as "the sphere doesn't
    // glow"). ES 3.2 avoided this by summing into a raw (unclamped) uint image
    // via imageAtomicAdd; ES 2.0 has no image load/store, so a float color
    // buffer is the direct equivalent when the driver supports one.
    void bind2DTextureToFrameBuffer(char* errorContext, bool highPrecision = false)
    {

        glGenFramebuffers(1, &frameBufferObject);
        glGenTextures(1, &outputTexture);
        glBindTexture(GL_TEXTURE_2D, outputTexture);

        bool useFloat = highPrecision &&
            epoxy_has_gl_extension("GL_OES_texture_float") &&
            epoxy_has_gl_extension("GL_EXT_color_buffer_float");

        // ES 2.0's unsized-internalformat upload (internalformat == format)
        // only guarantees a texture-filterable float format, not a
        // color-renderable one -- GL_EXT_color_buffer_float's renderability
        // guarantee is tied to the *sized* formats (GL_RGBA32F and friends),
        // which is why this needs the ES 3.x-style sized enum here even
        // though every other stage stays on the plain ES 2.0 unsized path.
        if (useFloat)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA,
                GL_FLOAT, NULL);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, NULL);
        // GL_OES_texture_float doesn't imply GL_LINEAR filtering support
        // (that's the separate GL_OES_texture_float_linear); GL_NEAREST is
        // always valid and this buffer is read back at its native resolution
        // by the very next stage, so there's nothing to smooth.
        GLint filter = useFloat ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
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