#ifndef RFUNC_SCRAMBLED_ZIPFIAN_H
#define RFUNC_SCRAMBLED_ZIPFIAN_H

#include <cmath>
#include <mutex>
#include <iostream>
#include "random.h"
#include "zipfian_int_distribution.h"

template<typename _IntType = int>
class scrambled_zipfian_int_distribution : zipfian_int_distribution<_IntType>
{

public:

  template<typename _UniformRandomBitGenerator>
  _IntType operator()(_UniformRandomBitGenerator &__urng)
  {
    long ret = zipfian_int_distribution<long>::next(__urng);
    long hash = fnvhash64(static_cast<uint64_t>(ret));
    ret = min + hash % itemcount;
    lastvalue = static_cast<_IntType>(ret);
    return static_cast<_IntType>(ret);
  }
  

  scrambled_zipfian_int_distribution(_IntType min_, _IntType max_, double zipfianconstant_ = zipfian_int_distribution<_IntType>::ZIPFIAN_CONSTANT)
  {
    min = min_;
    max = max_;
    itemcount = max - min + 1;
    if (zipfianconstant_ == USED_ZIPFIAN_CONSTANT)
    {
      zipfian_int_distribution<_IntType>::init(0, ITEM_COUNT, zipfianconstant_, ZETAN);
    }
    else
    {
      zipfian_int_distribution<_IntType>::init(0, ITEM_COUNT, zipfianconstant_, zipfian_int_distribution<_IntType>::zetastatic(max - min + 1, zipfianconstant_));
    }
  }

  scrambled_zipfian_int_distribution(const scrambled_zipfian_int_distribution &t) : zipfian_int_distribution<_IntType>(t)
  {
    min = t.min;
    max = t.max;
    itemcount = t.itemcount;
    lastvalue = t.lastvalue;
  }

public:
  //zipfian_int_distribution<_IntType> *gen;
  _IntType min, max, itemcount;
  _IntType lastvalue;

  static uint64_t fnvhash64(uint64_t val)
  {
    uint64_t hashval = FNV_OFFSET_BASIS_64;

    for (int i = 0; i < 8; i++)
    {
      uint64_t octet = val & 0x00ff;
      val = val >> 8;

      hashval = hashval ^ octet;
      hashval = hashval * FNV_PRIME_64;
    }
    return labs(hashval);
  }

private:

  static const uint64_t FNV_OFFSET_BASIS_64 = 0xCBF29CE484222325L;
  static const uint64_t FNV_PRIME_64 = 1099511628211L;
  const double USED_ZIPFIAN_CONSTANT = 0.99;
  const double ZETAN = 26.46902820178302;
  const long ITEM_COUNT = 10000000000L;
};

#endif