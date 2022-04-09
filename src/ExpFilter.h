/**
Quick and dirty Recursive Exponential Filter
*/

#ifndef EXPFILTER_h
#define EXPFILTER_h
#include <math.h> 
class ExpFilter {
  private:
    float x = 0; 
    float K = .1; // gain
    float tau; // filter time scale

  public:
    ExpFilter(float _tau) { tau = _tau; }

    /**
	float z - value
	float dT - delta time since last measurement 
    **/
    float add (float z, float dT) {
      K = K / (K + exp(-( dT )/tau));
      x = x + K * (z - x);
      return x;
    }
    inline float getX() {return x;}
};

#endif
