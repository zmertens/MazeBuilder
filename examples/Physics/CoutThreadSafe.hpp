#ifndef COUT_THREAD_SAFE_HPP
#define COUT_THREAD_SAFE_HPP        

#include <sstream>
#include <mutex>
#include <iostream>

// Source material here on async behavior
// https://github.com/PacktPublishing/Cpp17-STL-Cookbook/blob/master/Chapter09/chains.cpp
// Provides standard out in async context
struct CoutThreadSafe : public std::stringstream
{
private:
    static inline std::mutex cout_mutex;

public:
    ~CoutThreadSafe()
    {
        std::lock_guard<std::mutex> lock{cout_mutex};

        std::cout << rdbuf();

        std::cout.flush();
    }
};

#endif // COUT_THREAD_SAFE_HPP
