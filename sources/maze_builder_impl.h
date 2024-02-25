#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

#include <string>
#include "ibuilder.h"

class maze_builder_impl : public ibuilder {
public:
    maze_builder_impl(const std::string& description);

    void seed(unsigned int s);
    void interactive(bool i);
    void algo(const std::string& algo);
    void output(const std::string& filename);
    imaze_ptr build() override;
private:
    std::string m_description;
    unsigned int s;
    bool is_interactive;
    std::string algorithm;
    std::string filename;
};

#endif // MAZE_BUILDER_H