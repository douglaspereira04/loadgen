#ifndef CHAR_GENERATOR_H
#define CHAR_GENERATOR_H

#include <algorithm>
#include <functional>
#include <random>

#include "types.h"
#include "random.h"

class CharGenerator {
public:
    CharGenerator(long seed = std::mt19937::default_seed) {
        __generator =
            rfunc::uniform_distribution_rand(0, __CHARSET_LEN - 1, seed);
    }
    inline char operator()() { return __CHARSET[__generator()]; }

private:
    static const char __CHARSET[];
    static const size_t __CHARSET_LEN;
    rfunc::RandFunction __generator;
};

#endif
