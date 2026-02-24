#ifndef RFUNC_ACKNOWLEDGED_COUNTER_H
#define RFUNC_ACKNOWLEDGED_COUNTER_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

template <typename _IntType = int> class acknowledged_counter {

public:
    _IntType operator()() { return next(); }

    acknowledged_counter(_IntType count_start) {
        counter_ = count_start;
        limit_ = count_start - 1;
        window_ = 0;
    }

    acknowledged_counter(acknowledged_counter &other) {
        counter_ = other.count_start;
        limit_ = other.limit_;
        window_ = other.window_;
    }

    _IntType next() {
        counter_mutex_.lock();
        _IntType ret = counter_++;
        counter_mutex_.unlock();
        return ret;
    }

    /**
     * @brief Acknowledge the given value. Only affects last_value() after every
     * previous value to "value" param has been acknowledged.
     * @param value The value to acknowledge.
     */
    void acknowledge(_IntType value) {

        limit_mutex_.lock();
        if (value > limit_) {
            if (value > limit_) {
                _IntType increment = 0;
                uint_fast64_t mask = 0x1 << (value - limit_ - 1);
                window_ |= mask;
                while (window_ & 0x1) {
                    window_ = window_ >> 1;
                    increment++;
                }
                limit_ += increment;
            }
        }
        limit_mutex_.unlock();
    }

    _IntType last_value() {
        limit_mutex_.lock_shared();
        _IntType last = limit_;
        limit_mutex_.unlock_shared();
        return last;
    }

public:
    _IntType counter_;
    std::shared_mutex limit_mutex_;
    std::mutex counter_mutex_;
    uint_fast64_t window_;
    _IntType limit_;
};

#endif