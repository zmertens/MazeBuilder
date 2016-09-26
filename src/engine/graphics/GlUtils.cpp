#include "GlUtils.hpp"

#include "../Utils.hpp"

/**
 * @brief GlUtils::checkForOpenGLError
 * @param file
 * @param line
 * @return
 */
bool GlUtils::CheckForOpenGLError(const std::string& file, int line)
{
    bool error = false;
    GLenum glErr;
    glErr = glGetError();
    while(glErr != GL_NO_ERROR)
    {
        std::string message = "";
        switch(glErr)
        {
            case GL_INVALID_ENUM:
                message = "Invalid enum";
                break;
            case GL_INVALID_VALUE:
                message = "Invalid value";
                break;
            case GL_INVALID_OPERATION:
                message = "Invalid operation";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                message = "Invalid framebuffer operation";
                break;
            case GL_OUT_OF_MEMORY:
                message = "Out of memory";
                break;
            default:
                message = "Unknown error";
        }

        std::string fileError = "glError in file " + file + " @ line " + Utils::toString(line) + ", error message: " +  message;
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, fileError.c_str());
        glErr = glGetError();
        error = !error;
    }
    return error;
}

/**
 * @brief GlUtils::GlDebugCallback
 * @param source
 * @param type
 * @param id
 * @param severity
 * @param length
 * @param msg
 * @param param
 */
void GlUtils::GlDebugCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* msg, const void* param)
{
    // std::string sourceStr;
    // switch (source)
    // {
    //     case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    //         sourceStr = "WindowSys";
    //         break;
    //     case GL_DEBUG_SOURCE_APPLICATION:
    //         sourceStr = "App";
    //         break;
    //     case GL_DEBUG_SOURCE_API:
    //         sourceStr = "OpenGL";
    //         break;
    //     case GL_DEBUG_SOURCE_SHADER_COMPILER:
    //         sourceStr = "ShaderCompiler";
    //         break;
    //     case GL_DEBUG_SOURCE_THIRD_PARTY:
    //         sourceStr = "3rdParty";
    //         break;
    //     case GL_DEBUG_SOURCE_OTHER:
    //         sourceStr = "Other";
    //         break;
    //     default:
    //         sourceStr = "Unknown";
    // }

    // std::string typeStr;
    // switch (type)
    // {
    //     case GL_DEBUG_TYPE_ERROR:
    //         typeStr = "Error";
    //         break;
    //     case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    //         typeStr = "Deprecated";
    //         break;
    //     case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    //         typeStr = "Undefined";
    //         break;
    //     case GL_DEBUG_TYPE_PORTABILITY:
    //         typeStr = "Portability";
    //         break;
    //     case GL_DEBUG_TYPE_PERFORMANCE:
    //         typeStr = "Performance";
    //         break;
    //     case GL_DEBUG_TYPE_MARKER:
    //         typeStr = "Marker";
    //         break;
    //     case GL_DEBUG_TYPE_PUSH_GROUP:
    //         typeStr = "PushGrp";
    //         break;
    //     case GL_DEBUG_TYPE_POP_GROUP:
    //         typeStr = "PopGrp";
    //         break;
    //     case GL_DEBUG_TYPE_OTHER:
    //         typeStr = "Other";
    //         break;
    //     default:
    //         typeStr = "Unknown";
    // }

    // std::string sevStr;
    // switch (severity)
    // {
    //     case GL_DEBUG_SEVERITY_HIGH:
    //         sevStr = "HIGH";
    //         break;
    //     case GL_DEBUG_SEVERITY_MEDIUM:
    //         sevStr = "MED";
    //         break;
    //     case GL_DEBUG_SEVERITY_LOW:
    //         sevStr = "LOW";
    //         break;
    //     case GL_DEBUG_SEVERITY_NOTIFICATION:
    //         sevStr = "NOTIFY";
    //         break;
    //     default:
    //         sevStr = "UNK";
    // }

    // std::string outStr = sourceStr + ":" + typeStr + "[" + sevStr + "]" + "("
    //     + Utils::toString(id) + ")" + ": " + msg;

    // if (severity != GL_DEBUG_SEVERITY_LOW)
    //     SDL_LogDebug(SDL_LOG_CATEGORY_CUSTOM, outStr.c_str());
} // debugCallback()
