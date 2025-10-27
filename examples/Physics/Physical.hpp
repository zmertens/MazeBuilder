#ifndef PHYSICAL_HPP
#define PHYSICAL_HPP

#include <memory>

/// @brief Abstract base class for physical objects with update behavior
class Physical {
private:
    struct PhysicalConcept {
        virtual ~PhysicalConcept() = default;
        virtual void update(float elapsed) noexcept = 0;
        virtual std::unique_ptr<PhysicalConcept> clone() const = 0;
    };
    
    template<typename T>
    struct PhysicalModel : PhysicalConcept {
        T obj;
        
        PhysicalModel(T t) : obj(std::move(t)) {}
        
        void update(float elapsed) noexcept override {
            obj.update(elapsed);
        }
        
        std::unique_ptr<PhysicalConcept> clone() const override {
            return std::make_unique<PhysicalModel<T>>(obj);
        }
    };
    
    std::unique_ptr<PhysicalConcept> impl;

public:
    template<typename T>
    Physical(T obj) : impl(std::make_unique<PhysicalModel<T>>(std::move(obj))) {}
    
    Physical(const Physical& other) : impl(other.impl->clone()) {}
    Physical& operator=(const Physical& other) {
        if (this != &other) {
            impl = other.impl->clone();
        }
        return *this;
    }
    
    Physical(Physical&&) = default;
    Physical& operator=(Physical&&) = default;
    
    void update(float elapsed) noexcept {
        impl->update(elapsed);
    }
};

#endif // PHYSICAL_HPP