#ifndef BLOOM_EFFECTS_H
#define BLOOM_EFFECTS_H

#include "craft_types.h"
#include "gl_types.h"

class BloomEffects {
private:
    gl_framebuffer_handle m_fbo_hdr, m_fbo_pingpong[2], m_fbo_final;
    gl_renderbuffer_handle m_rbo_bloom_depth;
    gl_texture_handle m_color_buffers[2], m_color_buffers_pingpong[2], m_color_final;
    bool m_first_iteration, m_horizontal_blur;
    
public:
    BloomEffects();
    ~BloomEffects();
    
    void generateFramebuffers(int w, int h);
    void reset();
    void checkFramebuffer();
    
    void beginHDRPass();
    void processBloom(gl_vertex_array_handle quad_vao, const CraftAttrib& blur_attrib);
    void finalizeToTexture(gl_vertex_array_handle quad_vao, const CraftAttrib& screen_attrib, 
                          bool apply_bloom, float exposure);
    
    gl_texture_handle getFinalTexture() const { return m_color_final; }
};

#endif // BLOOM_EFFECTS_H
