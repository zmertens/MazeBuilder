#ifndef GL_BUFFER_MANAGER_H
#define GL_BUFFER_MANAGER_H

#include "gl_types.h"

#include <cstddef>

class GLBufferManager {
public:
    // Buffer creation and management - no OpenGL types in interface
    static gl_buffer_handle createBuffer(gl_sizei size, const gl_float* data);
    static void deleteBuffer(gl_buffer_handle buffer);
    static gl_float* allocateFaces(std::size_t components, std::size_t faces);
    static gl_buffer_handle createFacesBuffer(gl_sizei components, gl_sizei faces, gl_float* data);
    
    // Specific buffer generators - only craft types in interface
    static gl_buffer_handle createCrosshairBuffer(int scene_w, int scene_h, int scale);
    static gl_buffer_handle createWireframeBuffer(float x, float y, float z, float n);
    static gl_buffer_handle createCubeBuffer(float x, float y, float z, float n, int w);
    static gl_buffer_handle createPlantBuffer(float x, float y, float z, float n, int w);
    static gl_buffer_handle createPlayerBuffer(float x, float y, float z, float rx, float ry);
    static gl_buffer_handle createTextBuffer(float x, float y, float n, const char* text);
};

#endif // GL_BUFFER_MANAGER_H