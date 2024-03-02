#ifndef IBUILDER_H
#define IBUILDER_H

#include <memory>

#include "imaze.h"

class imaze_builder {
public:
    virtual imaze::imaze_ptr build() = 0;
};

#endif // IBUILDER_H