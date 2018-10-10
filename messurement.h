#ifndef MESSUREMENT_H
#define MESSUREMENT_H

#include <cstdint>
#include <limits>
#include <array>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "config.h"

template<size_t C> class messurement
{
private:
  std::array<double,C> data{}; //preinitialized Array
  size_t index;

  uint8_t valid;

  double value;
  double min;
  double mean;
  double max;

  std::chrono::high_resolution_clock::time_point hrt1;
  std::chrono::high_resolution_clock::time_point hrt2;

public:
  messurement():index(0),valid(0),
                value(0),min(std::numeric_limits<double>::max()),mean(DINTERVAL),max(0),
                hrt1(std::chrono::high_resolution_clock::now()),hrt2(std::chrono::high_resolution_clock::now()){
    return;
  }

  void take(){
    //Take current Time
    hrt2 = std::chrono::high_resolution_clock::now();

    //Messure Precision with stdlib
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(hrt2 - hrt1);
    hrt1 = hrt2; //Prepare next cycle

    value = time_span.count();
    data[index++] = value;

    if(RUNTIME_EVAL){
      //Monitor Min and Max
      if((valid>=10) && (value<min)){
        min = value;
      }
      if((valid>=10) && (value>max)){
        max = value;
      }

      //Calculate Mean Cycle Time
      if(valid>=10){
        mean = mean + ((value - mean)/2);
      }

      //Prepare next cycle
      if(valid<10)++valid;
    }
    return;
  }

  void result_cycle(struct timespec& ts, std::stringstream& ss){
    if(RUNTIME_EVAL && LOG_CYCLES){
      ss << std::scientific;
      ss.precision(6);
      ss << "sec: " << std::setw(8) << ts.tv_sec << " - nsec: " << std::setw(10) << ts.tv_nsec << " --> "
         << std::setw(11) << value << "[s]" << " --> "
         << std::setw(11) << min  << " [s]" << " | "
         << std::setw(11) << mean << " [s]" << " | "
         << std::setw(11) << max  << " [s]" << " { "
         << ((value>DINTERVAL)?("++"):("--")) << " } " << "\n";
    }
    return;
  }

  void result_offline(std::stringstream& ss){
    ss << std::scientific;
    ss.precision(6);

    uint8_t valid = 0;
    for(double _v:data){

      //Monitor Min and Max
      if((valid>=1) && (_v<min)){
        min = _v;
      }
      if((valid>=1) && (_v>max)){
        max = _v;
      }

      //Calculate Mean Cycle Time
      mean = mean + ((_v - mean)/2);

      ss << std::setw(11) << _v << "[s]" << " --> "
         << std::setw(11) << min  << " [s]" << " | "
         << std::setw(11) << mean << " [s]" << " | "
         << std::setw(11) << max  << " [s]" << " { "
         << ((_v>DINTERVAL)?("++"):("--")) << " } " << "\n";

      valid = uint8_t(valid<<1) | uint8_t(1); //Just prevent <1 (no efficency needed)
    }
    return;
  }

  void result_histogram(std::stringstream& ss){
    constexpr size_t const sidebins = 10;
    std::array<uint32_t,2*sidebins+1> hist{};

    double binsize = 0.005; //2.5%

    for(double _v:data){
      double x = _v - DINTERVAL;
      bool slower = (x<0)?(true):(false);
             x=std::abs(x);
      double y = x / (DINTERVAL*binsize);
      size_t z = size_t(std::lround(y));
      if(slower) z+=sidebins;
      else       z = (sidebins+1)-z;
      ++hist[z%hist.size()];
    }

    //Simple List Output
    ss << "|";
    for(uint32_t _v:hist){
      ss << _v << "|";
    }
    ss << "\n";
    return;
  }

};


#endif // MESSUREMENT_H
