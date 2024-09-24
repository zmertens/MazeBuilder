#ifndef OUTPUT_TYPES_ENUM_H
#define OUTPUT_TYPES_ENUM_H

namespace mazes {
    enum class output_types {
        PLAIN_TEXT,
        WAVEFRONT_OBJ_FILE,
        PNG,
        STDOUT,
        UNKNOWN
    };
}

#endif // OUTPUT_TYPES_ENUM_H