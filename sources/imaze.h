// Simply interface to specify that a maze runs an algorithm on itself
//  After the algorithm is run, check for success, and then output (interactive or not)

#ifndef IMAZE_H
#define IMAZE_H

#include <memory>

class imaze {
public:
    using imaze_ptr = std::unique_ptr<imaze>;
    virtual bool run() = 0;
};

#endif // IMAZE_H