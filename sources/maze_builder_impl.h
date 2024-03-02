#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

#include <string>
#include "imaze.h"
#include "imaze_builder.h"

class maze_builder_impl : public imaze_builder {
public:
    maze_builder_impl(const std::string& description);

    maze_builder_impl& seed(unsigned int s);
    maze_builder_impl& interactive(bool i);
    maze_builder_impl& algo(const std::string& algo);
    maze_builder_impl& output(const std::string& filename);
    imaze::imaze_ptr build() override;
private:
    std::string m_description;
    unsigned int s;
    bool is_interactive;
    std::string algorithm;
    std::string filename;
};

#endif // MAZE_BUILDER_H