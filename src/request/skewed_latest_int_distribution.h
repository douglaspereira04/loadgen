#ifndef RFUNC_SKEWED_LATEST_H
#define RFUNC_SKEWED_LATEST_H

#include <cmath>
#include <mutex> // std::mutex
#include "zipfian_int_distribution.h"
#include "acknowledged_counter.h"

template<typename _IntType = int>
class skewed_latest_int_distribution
{

public:

  template<typename _UniformRandomBitGenerator>
  _IntType operator()(_UniformRandomBitGenerator &__urng)
  {
    return next(__urng);
  }
  

  skewed_latest_int_distribution(acknowledged_counter<_IntType> *&basis, zipfian_int_distribution<_IntType> *&zipfian_int_distribution)
  {
    basis_ = basis;
    zipfian_int_distribution_ = zipfian_int_distribution;
  }

  template<typename _UniformRandomBitGenerator>
  _IntType next(_UniformRandomBitGenerator &__urng){
    _IntType max = basis_->last_value();
    _IntType next = max - zipfian_int_distribution_->next(__urng, max);
    last_value_ = next;
    return next;
  }


  skewed_latest_int_distribution(const skewed_latest_int_distribution &t)
  {
    zipfian_int_distribution_ = t.zipfian_int_distribution_;
    basis_ = t.basis_;
    last_value_ = t.last_value_;
  }

public:

  zipfian_int_distribution<_IntType> *zipfian_int_distribution_;
  acknowledged_counter<_IntType> *basis_;
  _IntType last_value_;
};

#endif