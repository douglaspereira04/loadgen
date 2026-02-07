#include "random.h"

namespace rfunc {

Distribution str_to_dist(std::string str) { return __STR_TO_DIST.at(str); }

RandFunction uniform_distribution_rand(long min_value, long max_value,
                                       long seed) {
    std::mt19937 generator(seed);
    std::uniform_int_distribution<long> distribution(min_value, max_value);

    return std::bind(distribution, generator);
}

DoubleRandFunction uniform_double_distribution_rand(double min_value,
                                                    double max_value,
                                                    long seed) {
    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(min_value, max_value);

    return std::bind(distribution, generator);
}

RandFunction fixed_distribution(int value) {
    return [value]() { return value; };
}

RandFunction binomial_distribution(int n_experiments,
                                   double success_probability, long seed) {
    std::mt19937 generator(seed);
    std::binomial_distribution<long> distribution(n_experiments,
                                                  success_probability);

    return std::bind(distribution, generator);
}

RandFunction zipfian_distribution(long min, long max, long seed) {
    std::mt19937 generator(seed);
    zipfian_int_distribution<long> distribution(min, max);
    return std::bind(distribution, generator);
}

RandFunction scrambled_zipfian_distribution(long min, long max, long seed) {
    std::mt19937 generator(seed);
    scrambled_zipfian_int_distribution<long> distribution(min, max);
    return std::bind(distribution, generator);
}

RandFunction skewed_latest_distribution(acknowledged_counter<long> *&counter,
                                        zipfian_int_distribution<long> *&zip,
                                        long seed) {
    std::mt19937 generator(seed);
    skewed_latest_int_distribution<long> distribution(counter, zip);
    return std::bind(distribution, generator);
}

RandFunction ranged_binomial_distribution(int min_value, int n_experiments,
                                          double success_probability,
                                          long seed) {
    auto random_func = binomial_distribution(n_experiments - min_value,
                                             success_probability, seed);

    return [min_value, random_func]() {
        auto value = random_func() + min_value;
        return value;
    };
}

} // namespace rfunc
