#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include "craft_types.h"

class GLRenderer {
public:
    // Drawing functions - use craft types, no OpenGL types exposed
    static void drawTriangles3D_AO(const CraftAttrib* attrib, gl_buffer_handle buffer, int count);
    static void drawTriangles3D_Text(const CraftAttrib* attrib, gl_buffer_handle buffer, int count);
    static void drawTriangles3D(const CraftAttrib* attrib, gl_buffer_handle buffer, int count);
    static void drawTriangles2D(const CraftAttrib* attrib, gl_buffer_handle buffer, gl_sizei count);
    static void drawLines(const CraftAttrib* attrib, gl_buffer_handle buffer, int components, int count);
    
    // High-level rendering functions
    static void renderCrosshairs(const CraftAttrib* attrib, const RenderParams& params);
    static void renderWireframe(const CraftAttrib* attrib, const RenderParams& params, int hx, int hy, int hz);
    static void renderItem(const CraftAttrib* attrib, gl_texture_handle texture, 
                          const RenderParams& params, int item_index);
    static void renderText(const CraftAttrib* attrib, gl_texture_handle font, 
                          const RenderParams& params, int justify, float x, float y, float n, const char* text);
};

#endif // GL_RENDERER_H
