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

    // ES rejects linking a program that has no fragment shader; the matching
    // FragmentShader attaches next and does the link for this program.

    glGenVertexArrays(1, &vertexArrayObject);
    glGenBuffers(1, &vertexBufferObject);

    glBindVertexArray(vertexArrayObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDisableVertexAttribArray(0);

    glBindVertexArray(0);
    glUseProgram(0);

    if (args != NULL)
        delete args;
}

void VertexShader::draw(unsigned int* texture)
{
    glBindVertexArray(vertexArrayObject);
    if (texture != NULL)
        glBindTexture(GL_TEXTURE_2D, *texture);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
}

VertexShader::~VertexShader()
{
    glDeleteBuffers(1, &vertexBufferObject);
    glDeleteVertexArrays(1, &vertexArrayObject);
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

    int numAtomicTextures = 0;

    unsigned int* atomicImageTexture = NULL;

    if (args != NULL && ((FragmentShaderCompilationArgs*)args)->atomicImageTexture != NULL) {
        numAtomicTextures = ((FragmentShaderCompilationArgs*)args)->numAtomicTextures;

        atomicImageTexture = ((FragmentShaderCompilationArgs*)args)->atomicImageTexture;
    }

    unsigned int imageUnit = 0;

    for (int i = 0; i < numAtomicTextures; i++) {

        glGenTextures(1, &atomicImageTexture[i]);
        glBindTexture(GL_TEXTURE_2D, atomicImageTexture[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, windowWidth, windowHeight);

        glBindImageTexture(imageUnit, atomicImageTexture[i], 0, GL_FALSE, 0,
            GL_READ_WRITE, GL_R32UI);

        auto uniformLocationsIterator = (*args->uniformLocations).find(std::string("atomicImageTexture" + std::to_string(i)).c_str());
        if (uniformLocationsIterator == (*args->uniformLocations).end()) {
            imageUnit++;
            continue;
        }
        GLint imageLocation = uniformLocationsIterator->second;

        glProgramUniform1i(*glProgram, imageLocation, imageUnit);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atomicImageTexture[i]);

        imageUnit++;
    }

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

