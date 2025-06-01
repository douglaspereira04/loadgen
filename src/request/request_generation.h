#ifndef WORKLOAD_REQUEST_GENERATOR_H
#define WORKLOAD_REQUEST_GENERATOR_H

#include <algorithm>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "toml.hpp"
#include "types.h"
#include "random.h"

namespace workload {
    using namespace rfunc;


void create_requests(std::string config_path);

Type next_operation(
    std::vector<std::pair<Type,double>> values, 
    rfunc::DoubleRandFunction *generator
);

class CharGenerator{
public:
    CharGenerator(long seed = std::mt19937::default_seed){
        __generator = uniform_distribution_rand(0, __CHARSET_LEN-1, seed);
    }
    inline char operator()(){
        return __CHARSET[__generator()];
    }
private:
    static const char __CHARSET[];
    static const size_t __CHARSET_LEN;
    RandFunction __generator;
};

size_t gen_value(char* out_str, CharGenerator *char_generator, RandFunction *len_generator, size_t min_len, size_t max_len);


}


#endif
