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
        virtual std::unique_ptr<DrawableConcept> clone() const = 0;
    };
    
    template<typename T>
    struct DrawableModel : DrawableConcept {
        T obj;
        
        DrawableModel(T t) : obj(std::move(t)) {}

        void draw(RenderStates states) const noexcept override {
            obj.draw(states);
        }
        
        std::unique_ptr<DrawableConcept> clone() const override {
            return std::make_unique<DrawableModel<T>>(obj);
        }
    };
    
    std::unique_ptr<DrawableConcept> impl;

public:
    explicit Drawable() = default;

    template<typename T>
    Drawable(T obj) : impl(std::make_unique<DrawableModel<T>>(std::move(obj))) {}
    
    Drawable(const Drawable& other) : impl(other.impl->clone()) {}
    
    Drawable& operator=(const Drawable& other) {
        if (this != &other) {
            impl = other.impl->clone();
        }
        return *this;
    }
    
    Drawable(Drawable&&) = default;
    Drawable& operator=(Drawable&&) = default;
    
    void draw(RenderStates states) const noexcept {
        impl->draw(states);
    }
};
#endif // DRAWABLE_HPP
