#include "shader_files.h"
#include "embedded_resources.h"
#include "errors.h"
#include <vector>

std::string ShaderFiles::LoadFile(std::string fileName)
{
    auto it = EmbeddedResources::files.find(fileName);
    if (it == EmbeddedResources::files.end())
        return "";

    return it->second;
}

std::string ShaderFiles::extractDirectory(std::string fileName, bool isFile)
{
    std::stringstream ss(fileName);
    std::string token;
    std::vector<std::string> tokens;
    char delimiter = '/';

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    std::vector<std::string> directoryVector;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (!isFile && i == tokens.size() - 1)
            continue;

        if (tokens.at(i) == "..") {
            if (!directoryVector.empty())
                directoryVector.pop_back();
        } else
            directoryVector.push_back(tokens.at(i) + (i == tokens.size() - 1 ? "" : "/"));
    }

    std::string directory = "";

    for (const auto& t : directoryVector) {
        directory += t;
    }

    return directory;
}

std::string ShaderFiles::process_includes(std::string content, std::string directory, std::string configName, std::map<std::string, int>& vars)
{
    if (content.empty())
        return "";

    const std::string include_directive = "#include";
    std::string result = "", wildCardName = ":$CONFIGFILE";
    std::istringstream f(content);
    std::string line;
    while (std::getline(f, line)) {
        std::string::size_type start_pos = 0;
        std::string modifiedLine = line + "\n";

        while (std::string::npos != (start_pos = line.find(include_directive, start_pos))) {
            std::string::size_type startIdx = line.find("\"", start_pos);
            if (startIdx == std::string::npos)
                break;

            std::string::size_type endIdx = line.find("\"", startIdx + 1);
            if (endIdx == std::string::npos)
                break;

            std::string fileName = line.substr(startIdx + 1, endIdx - startIdx - 1);

            if (fileName == wildCardName) {
                fileName = configName;
                directory = "";
            }

            else if (fileName == wildCardName.substr(1)) {
                fileName = configName;
            }

            if (fileName[0] == ':') // Absolute Import
            {

                fileName = fileName.substr(1);
            } else
                fileName = directory + fileName; // Relative Import

            fileName = extractDirectory(fileName, true);

            std::string includedFile = LoadFile(fileName);

            if (includedFile.empty())
                Errors::throwError("does not exist, or has empty contents!", fileName);

            std::string extractedDirectory = extractDirectory(fileName, false);

            includedFile = process_includes(includedFile, extractedDirectory, configName, vars);
            modifiedLine = includedFile;
            break;
        }

        result += modifiedLine;
    }
    return result;
}

std::string ShaderFiles::process_expands(std::string content, const std::map<std::string, int>& variables, std::string errorContext)
{

    if (content.empty())
        return "";

    const std::string expand_directive = "#expand";
    std::string result = "";
    std::istringstream f(content);
    std::string line;

    while (std::getline(f, line)) {
        std::string::size_type start_pos = line.find(expand_directive);
        std::string modifiedLine = line + "\n";

        if (start_pos != std::string::npos) {
            // Everything after "#expand ": one or more whitespace-separated tokens
            // where the LAST token is the count (a digit literal or a variable),
            // and the tokens before it form the body. A body containing '#' is a
            // template: '#' is replaced by the index and the line emitted verbatim
            // (GLSL ES has no '##' paste operator). Otherwise the body is a
            // function name and we emit `body(i);` as before.
            std::string rest = line.substr(start_pos + expand_directive.length());

            std::string::size_type lastSpace = rest.find_last_not_of(" \t");
            rest = rest.substr(0, lastSpace + 1);
            lastSpace = rest.find_last_of(" \t");

            std::string countToken = rest.substr(lastSpace + 1);
            std::string body = rest.substr(0, lastSpace);
            body = body.substr(body.find_first_not_of(" \t"));

            int value;
            if (std::isdigit((unsigned char)countToken[0])) {
                value = std::stoi(countToken);
            } else {
                auto it = variables.find(countToken);
                if (it == variables.end()) {
                    Errors::throwError("Variable '" + countToken + "' not found for expansion.", errorContext, "In");
                    result += modifiedLine;
                    continue;
                }
                value = it->second;
            }

            bool isTemplate = body.find('#') != std::string::npos;

            std::string expanded = "";
            for (int i = 0; i < value; i++) {
                if (isTemplate) {
                    std::string t = body;
                    for (std::string::size_type p = t.find('#'); p != std::string::npos; p = t.find('#', p))
                        t.replace(p, 1, std::to_string(i));
                    expanded += t + "\n";
                } else {
                    expanded += body + "(" + std::to_string(i) + ");\n";
                }
            }

            modifiedLine = expanded;
        }

        result += modifiedLine;
    }

    return result;
}

void ShaderFiles::loadShaders(std::string shaderName, std::string configName, enum ShaderTypes shaderType, std::map<std::string, int> vars)
{

    ShaderFiles *shaderFile = this, *prevFile = NULL;
    std::string directory;
    int idx = 1;

    std::string shaderStringFormat = "";

    // Flat layout: shaders are named "<shaderName>-<idx>.<ext>" directly under shaders/.
    directory = shaderName + "-";
    shaderStringFormat = shaderType == VERTEX ? ".vert" : ".frag";
    do {
        shaderFile->fileContent = shaderFile->LoadFile(std::string(directory + std::to_string(idx) + shaderStringFormat));

        if (shaderFile->fileContent.empty()) {
            if (idx == 1 && shaderType == ShaderTypes::FRAGMENT)
                Errors::throwError("Shader '" + shaderName + "' not found.", "", "");

            if (prevFile != NULL) {
                free(prevFile->next);
                prevFile->next = NULL;
            }

            return;
        }

        shaderFile->fileContent = shaderFile->process_includes(shaderFile->fileContent, directory, configName, vars);
        shaderFile->fileContent = shaderFile->process_expands(shaderFile->fileContent, vars, shaderName);

        shaderFile->next = new ShaderFiles;
        prevFile = shaderFile;
        shaderFile = shaderFile->next;
        idx++;
    } while (true);
}
