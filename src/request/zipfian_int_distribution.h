#ifndef RFUNC_ZIPFIAN_H
#define RFUNC_ZIPFIAN_H

#include <cmath>
#include <mutex>

template<typename _IntType = int>
class zipfian_int_distribution
{

public:

  template<typename _UniformRandomBitGenerator>
  _IntType operator()(_UniformRandomBitGenerator &__urng)
  {
    return next(__urng);
  }

  zipfian_int_distribution(){}

  zipfian_int_distribution(_IntType min, _IntType max, double zipfian_constant, double zetan_)
  {
    init(min, max, zipfian_constant, zetan_);
  }

  zipfian_int_distribution(_IntType min, _IntType max)
  {
    init(min, max);
  }

  zipfian_int_distribution(const zipfian_int_distribution &t)
  {
    items = t.items;
    base = t.base;
    zipfianconstant = t.zipfianconstant;
    alpha = t.alpha;
    zetan = t.zetan;
    eta = t.eta;
    theta = t.theta;
    zeta2theta = t.zeta2theta;
    countforzeta = t.countforzeta;
    allowitemcountdecrease = t.allowitemcountdecrease;
    lastvalue = t.lastvalue;
  }

  template<typename _UniformRandomBitGenerator>
  _IntType next(_UniformRandomBitGenerator &__urng){
    return next(__urng, items);
  }
  
  template<typename _UniformRandomBitGenerator>
  _IntType next(_UniformRandomBitGenerator &__urng, _IntType itemcount)
  {

		constexpr auto __urngmin = _UniformRandomBitGenerator::min();
		constexpr auto __urngmax = _UniformRandomBitGenerator::max();

    if (itemcount != countforzeta)
    {
      if (itemcount > countforzeta)
      {
        zetan = zeta(countforzeta, itemcount, theta, zetan);
        eta = (1 - pow(2.0 / items, 1 - theta)) / (1 - zeta2theta / zetan);
      }
      else if ((itemcount < countforzeta) && (allowitemcountdecrease))
      {
        zetan = zeta(itemcount, theta);
        eta = (1 - pow(2.0 / items, 1 - theta)) / (1 - zeta2theta / zetan);
      }
    }

    double random = (double)__urng();
    double u = (double)((random-__urngmin)/(__urngmax-__urngmin));
    double uz = u * zetan;

    if (uz < 1.0)
    {
      return base;
    }

    if (uz < 1.0 + pow(0.5, theta))
    {
      return base + 1;
    }

    _IntType ret = base + (_IntType)((itemcount)*pow(eta * u - eta + 1, alpha));
    lastvalue = ret;
    return ret;
  }
  
  protected:
  void init(_IntType min, _IntType max, double zipfian_constant, double zetan_)
  {
    items = max - min + 1;
    base = min;
    zipfianconstant = zipfian_constant;
    theta = zipfianconstant;
    zeta2theta = zeta(2, theta);
    alpha = 1.0 / (1.0 - theta);
    zetan = zetan_;
    countforzeta = items;
    eta = (1 - pow(2.0 / items, 1 - theta)) / (1 - zeta2theta / zetan);
  }

  void init(_IntType min, _IntType max){
    init(min, max, ZIPFIAN_CONSTANT, zetastatic(max - min + 1, ZIPFIAN_CONSTANT));
  }
  
  double zeta(long n, double thetaVal)
  {
    countforzeta = n;
    return zetastatic(n, thetaVal);
  }

  static double zetastatic(long n, double theta)
  {
    return zetastatic(0, n, theta, 0);
  }

  double zeta(long st, long n, double thetaVal, double initialsum)
  {
    countforzeta = n;
    return zetastatic(st, n, thetaVal, initialsum);
  }

  static double zetastatic(long st, long n, double theta, double initialsum)
  {
    double sum = initialsum;
    for (long i = st; i < n; i++)
    {

      sum += 1 / (pow(i + 1, theta));
    }
    return sum;
  }


  //rfunc::DoubleRandFunction uniform = rfunc::uniform_double_distribution_rand(0.0, 1.0);
public:
  static constexpr double ZIPFIAN_CONSTANT = 0.99;

protected:

  _IntType items;
  _IntType base;
  double zipfianconstant;
  double alpha, zetan, eta, theta, zeta2theta;
  _IntType countforzeta;
  bool allowitemcountdecrease = false;
  _IntType lastvalue;
};

#endif