#ifndef IBUILDER_H
#define IBUILDER_H

#include <memory>

#include "imaze.h"

class ibuilder {
public:
    using imaze_ptr = std::unique_ptr<imaze>;

    virtual imaze_ptr build() = 0;
};

#endif // IBUILDER_H