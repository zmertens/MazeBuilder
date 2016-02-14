#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include "../SdlManager.hpp"

namespace ShaderTypes
{
const int VERTEX_SHADER = 0;
const int FRAGMENT_SHADER = 1;
const int GEOMETRY_SHADER = 2;
const int TESSELATION_EVAL_SHADER = 3;
const int TESSELATION_CONTROL_SHADER = 4;
const int COMPUTE_SHADER = 5;
}

class Shader final
{
public:
    typedef std::unique_ptr<Shader> Ptr;
public:
    explicit Shader(const SdlManager& sdlManager);
    virtual ~Shader();

    void compileAndAttachShader(const int shaderType, const std::string& filename);
    void compileAndAttachShader(const int shaderType, const std::string& codeId, const GLchar* code);
    void linkProgram();
    void bind() const;
    void release() const;

    void cleanUp();

    std::string getGlslUniforms() const;
    std::string getGlslAttribs() const;

    void setUniform(const std::string& str, const glm::mat3& matrix);
    void setUniform(const std::string& str, const glm::mat4& matrix);
    void setUniform(const std::string& str, const glm::vec2& vec);
    void setUniform(const std::string& str, const glm::vec3& vec);
    void setUniform(const std::string& str, const glm::vec4& vec);
    void setUniform(const std::string& str, GLfloat arr[][2], unsigned int count);
    void setUniform(const std::string& str, GLint arr[], unsigned int count);
    void setUniform(const std::string& str, GLfloat arr[], unsigned int count);
    void setUniform(const std::string& str, GLfloat value);
    void setUniform(const std::string& str, GLdouble value);
    void setUniform(const std::string& str, GLint value);
    void setUniform(const std::string& str, GLuint value);

    void setSubroutine(GLenum shaderType, GLuint count, const std::string& name);
    void setSubroutine(GLenum shaderType, GLuint count, GLuint index);

    void bindFragDataLocation(const std::string& str, GLuint loc);
    void bindAttribLocation(const std::string& str, GLuint loc);

    unsigned int getProgramHandle() const;
    GLenum getShaderType(const int shaderType) const;
    const SdlManager& getSdlManager() const;
    std::unordered_map<std::string, GLint> getGlslLocations() const;
    std::unordered_map<int, std::string> getFileNames() const;

private:
    const SdlManager& cSdlManager;
    GLint mProgram;
    std::unordered_map<std::string, GLint> mGlslLocations;
    std::unordered_map<int, std::string> mFileNames;
private:
    Shader(const Shader& other);
    Shader& operator=(const Shader& other);
    GLuint compile(const int shaderType, const std::string& shaderCode);
    GLuint compile(const int shaderType, const GLchar* shaderCode);
    void attach(GLuint shaderId);
    void createProgram();
    void deleteShader(GLuint shaderId);
    void deleteProgram(GLint shaderId);
    GLint getUniformLocation(const std::string& str);
    GLint getAttribLocation(const std::string& str);
    GLuint getSubroutineLocation(GLenum shaderType, const std::string& name);
    std::string getStringFromType(GLenum shaderType) const;
};

#endif // SHADER_HPP
