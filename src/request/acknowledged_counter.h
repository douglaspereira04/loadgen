#ifndef RFUNC_ACKNOWLEDGED_COUNTER_H
#define RFUNC_ACKNOWLEDGED_COUNTER_H

#include <cmath>
#include "random.h"
#include <stdexcept>

template<typename _IntType = int>
class acknowledged_counter
{

public:

  _IntType operator()()
  {
    return next();
  }
  

  acknowledged_counter(_IntType count_start)
  {
    counter_ = count_start;
    limit_ = count_start-1;
  }

  acknowledged_counter(acknowledged_counter &other)
  {
    counter_ = other.count_start;
    limit_ = other.limit_;
  }

  _IntType next(){
    return counter_++;
  }


  void acknowledge(_IntType value) {
    //ycsb like acknowledgement window mechanism is not adequate on pre generated workload
    if(value > limit_){
      limit_ = value;
    }
  }

  _IntType last_value(){
    return limit_;
  }
  
public:
  _IntType counter_;

  _IntType limit_;
};

#endif