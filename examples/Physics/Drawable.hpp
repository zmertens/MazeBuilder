#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include "RenderStates.hpp"

#include <memory>

/// @brief Abstract base class for drawable objects
class Drawable {
private:
    struct DrawableConcept {
        virtual ~DrawableConcept() = default;
        virtual void draw(RenderStates states) const noexcept = 0;
        // Remove clone method as it's not needed for move-only semantics
    };
    
    template<typename T>
    struct DrawableModel : DrawableConcept {
        T obj;
        
        DrawableModel(T t) : obj(std::move(t)) {}

        void draw(RenderStates states) const noexcept override {
            obj.draw(states);
        }
    };
    
    std::unique_ptr<DrawableConcept> impl;

public:
    explicit Drawable() = default;

    template<typename T>
    Drawable(T obj) : impl(std::make_unique<DrawableModel<T>>(std::move(obj))) {}
    
    // Delete copy constructor and assignment operator
    Drawable(const Drawable&) = delete;
    Drawable& operator=(const Drawable&) = delete;
    
    // Allow move constructor and assignment operator
    Drawable(Drawable&&) = default;
    Drawable& operator=(Drawable&&) = default;
    
    void draw(RenderStates states) const noexcept {
        impl->draw(states);
    }
};
#endif // DRAWABLE_HPP
