#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <memory>

class Generator {
public:
    Generator(const std::string& title, const std::string& version, int w, int h);
    ~Generator();

    // Delete copy constructor and copy assignment operator
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    // Default move constructor and move assignment operator
    Generator(Generator&&) = default;
    Generator& operator=(Generator&&) = default;

    bool run() const noexcept;

    // Singleton pattern
    static std::shared_ptr<Generator> get_instance(const std::string& title, const std::string& version, int w, int h) {
        static std::shared_ptr<Generator> instance = std::make_shared<Generator>(cref(title), std::cref(version), w, h);
        return instance;
    }
private:
    struct GeneratorImpl;
    std::unique_ptr<GeneratorImpl> m_impl;
};

#endif
