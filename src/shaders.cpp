#include "shaders.h"

VertexShader::VertexShader(std::string shaderSource, ShaderCompilationArgs* args)
{
    this->glProgram = args->glProgram;
    if (glProgram == NULL) {
        this->glProgram = new unsigned int;
        *this->glProgram = 0;
    }
    this->windowWidth = args->windowWidth;
    this->windowHeight = args->windowHeight;

    this->shaderSource = (shaderSource.empty() ? defaultVertexShaderSource : shaderSource);

    compileShaderSource(args);
}

void Shader::checkCompileErrors(std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shaderObject, 1024, NULL, infoLog);
            Errors::throwError("ERROR::SHADER_COMPILATION_ERROR of type: " + type + "\n" + infoLog + "\n -- --------------------------------------------------- -- ", "", "", 1);
        }
    } else {
        glGetProgramiv(*glProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(*glProgram, 1024, NULL, infoLog);
            Errors::throwError("ERROR::PROGRAM_LINKING_ERROR of type: " + type + "\n" + infoLog + "\n -- --------------------------------------------------- -- ", "", "", 1);
        }
    }
}

void VertexShader::compileShaderSource(

    ShaderCompilationArgs* args)
{

    char* charShaderSource = (char*)(shaderSource.c_str());

    shaderObject = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderObject, 1, &charShaderSource,
        NULL);

    glCompileShader(shaderObject);
    checkCompileErrors("VERTEX");

    if (*glProgram == 0)
        *glProgram = glCreateProgram();

    glAttachShader(*glProgram, shaderObject);

    // ES 2.0 has no `layout(location = ...)`, so pin attribute location 0 to
    // "aPos" explicitly before linking (the matching FragmentShader attaches
    // next and does the link). Every vertex shader in this codebase (the
    // default full-screen quad, ncs-1.vert, the blit shader) uses "aPos" for
    // its sole vertex attribute, so one bind covers all of them; a shader
    // without an "aPos" (there are none) would just make this a no-op.
    glBindAttribLocation(*glProgram, 0, "aPos");

    // ES 2.0 has no VAOs (core from ES 3.0); vertex attrib state is global
    // and gets re-bound on every draw() / drawPoints() call instead.
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);

    if (args != NULL)
        delete args;
}

void VertexShader::draw(unsigned int* texture)
{
    if (texture != NULL)
        glBindTexture(GL_TEXTURE_2D, *texture);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexShader::drawPoints(unsigned int pointVbo, int count)
{
    if (count <= 0)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, pointVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_POINTS, 0, count);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexShader::~VertexShader()
{
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteShader(shaderObject);
}

FragmentShader::
    FragmentShader(std::string shaderSource,
        ShaderCompilationArgs* args)
{
    this->glProgram = args->glProgram;
    if (glProgram == NULL) {
        this->glProgram = new unsigned int;
        *this->glProgram = 0;
    }
    this->windowWidth = args->windowWidth;
    this->windowHeight = args->windowHeight;
    this->shaderSource = shaderSource;
    compileShaderSource(args);
}

void FragmentShader::compileShaderSource(

    ShaderCompilationArgs* args)
{

    char* charShaderSource = (char*)shaderSource.c_str();

    shaderObject = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(shaderObject, 1, &charShaderSource,
        NULL);

    glCompileShader(shaderObject);
    checkCompileErrors("FRAGMENT");

    if (*glProgram == 0)
        *glProgram = glCreateProgram();

    glAttachShader(*glProgram, shaderObject);

    glLinkProgram(*glProgram);
    checkCompileErrors("PROGRAM");

    glUseProgram(*glProgram);

    int count;
    glGetProgramiv(*glProgram, GL_ACTIVE_UNIFORMS, &count);

    GLchar name[512];
    int length, size;
    GLenum type;

    for (int i = 0; i < count; i++) {
        glGetActiveUniform(*glProgram, (GLuint)i, 512, &length, &size, &type, name);
        (*args->uniformLocations)[name] = i;
    }

    // ES 3.2 bound the shared atomic image texture(s) here (glTexStorage2D /
    // glBindImageTexture). ES 2.0 has no image load/store; the particle stage
    // that used it now reads its input like any other chained stage (a plain
    // `tex` sampler bound by ShaderProgram::render(), see ncs-2.frag) and
    // writes via additive blending instead of imageAtomicAdd (see ncs-1.vert).

    glUseProgram(0);

    if (args != NULL)
        delete args;
}

void FragmentShader::updateUniforms(int windowWidth, int windowHeight, int leftAudio, int rightAudio, float ticks, unsigned int audioLSize, unsigned int audioLTexture, unsigned int audioRSize, unsigned int audioRTexture,
    std::map<std::string, int> uniformLocations)
{

    auto uniformLocationsIterator = uniformLocations.find("time");
    if (uniformLocationsIterator != uniformLocations.end())
        glUniform1f(uniformLocationsIterator->second, (float)ticks);

    uniformLocationsIterator = uniformLocations.find("resolution");
    if (uniformLocationsIterator != uniformLocations.end())
        glUniform2f(uniformLocationsIterator->second, windowWidth, windowHeight);

    uniformLocationsIterator = uniformLocations.find("audioRSize");
    if (uniformLocationsIterator != uniformLocations.end())
        glUniform1i(uniformLocationsIterator->second, rightAudio == 1 ? audioRSize : audioLSize);

    uniformLocationsIterator = uniformLocations.find("audioLSize");
    if (uniformLocationsIterator != uniformLocations.end())
        glUniform1i(uniformLocationsIterator->second, leftAudio == 0 ? audioLSize : audioRSize);

    uniformLocationsIterator = uniformLocations.find("audioR");
    if (uniformLocationsIterator != uniformLocations.end())

    {
        glActiveTexture(GL_TEXTURE0 + 1);

        glBindTexture(GL_TEXTURE_2D, rightAudio == 1 ? audioRTexture : audioLTexture);
        glUniform1i(uniformLocationsIterator->second, 1);

        glActiveTexture(GL_TEXTURE0);
    }

    uniformLocationsIterator = uniformLocations.find("audioL");
    if (uniformLocationsIterator != uniformLocations.end()) {
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, leftAudio == 0 ? audioLTexture : audioRTexture);
        glUniform1i(uniformLocationsIterator->second, 2);

        glActiveTexture(GL_TEXTURE0);
    }
}

FragmentShader::~FragmentShader()
{
    glDeleteShader(shaderObject);
    glDeleteTextures(1, &outputTexture);
    glDeleteFramebuffers(1, &frameBufferObject);
}

