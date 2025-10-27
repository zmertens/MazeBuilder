#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Renderer {
public:
    Renderer();
    ~Renderer();
    
private:
    struct RendererImpl;
    std::unique_ptr<RendererImpl> m_impl;
};

#endif // RENDERER_HPP