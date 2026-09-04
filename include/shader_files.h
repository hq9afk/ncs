#ifndef SHADER_FILES_H
#define SHADER_FILES_H

#include <map>
#include <string>

#include "shaders.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>

class ShaderFiles {
private:
    std::string LoadFile(std::string nm);

    std::string process_includes(std::string content, std::string directory, std::string configName, std::map<std::string, int>& vars);
    std::string process_expands(std::string content, const std::map<std::string, int>& variables, std::string errorContext);
    std::string extractDirectory(std::string fileName, bool isFile);

public:
    std::string fileContent;

    ShaderFiles* next = NULL;

    void loadShaders(std::string shaderName, std::string configName, enum ShaderTypes shaderType, std::map<std::string, int> vars);
};

#endif