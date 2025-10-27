#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include <memory>

/// @brief Abstract base class for drawable objects
class Drawable {
private:
    struct DrawableConcept {
        virtual ~DrawableConcept() = default;
        virtual void draw(float elapsed) const noexcept = 0;
        virtual std::unique_ptr<DrawableConcept> clone() const = 0;
    };
    
    template<typename T>
    struct DrawableModel : DrawableConcept {
        T obj;
        
        DrawableModel(T t) : obj(std::move(t)) {}

        void draw(float elapsed) const noexcept override {
            obj.draw(elapsed);
        }
        
        std::unique_ptr<DrawableConcept> clone() const override {
            return std::make_unique<DrawableModel<T>>(obj);
        }
    };
    
    std::unique_ptr<DrawableConcept> impl;

public:
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
    
    void draw(float elapsed) const noexcept {
        impl->draw(elapsed);
    }
};
#endif // DRAWABLE_HPP
