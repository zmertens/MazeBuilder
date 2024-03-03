#ifndef WRITER_H
#define WRITER_H

#include <string>
#include <future>

#include <tinyobjloader/tiny_obj_loader.h>

class writer {
public:
    bool write(const std::string& filename);
private:

};

#endif // WRITER_H
