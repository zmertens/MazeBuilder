#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include <string>

class output_writer {
public:
    output_writer(const std::string& out);
    bool write(const std::string& content) const;
private:
    const std::string& output_place;
};

#endif // OUTPUT_WRITER_H