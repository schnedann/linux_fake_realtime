#ifndef MESSUREMENT_H
#define MESSUREMENT_H

#include <cstdint>
#include <limits>
#include <array>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

#include "config.h"

template<size_t C> class messurement
{
private:
  std::array<double,C> data{};
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
                value(0),min(std::numeric_limits<double>::max()),mean(0),max(0),
                hrt1(std::chrono::high_resolution_clock::now()),hrt2(std::chrono::high_resolution_clock::now()){
    return;
  }

  void take(){
    hrt2 = std::chrono::high_resolution_clock::now();

    //Messure Precision with stdlib
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(hrt2 - hrt1);
    hrt1 = hrt2; //Prepare next cycle

    value = time_span.count();
    data[index++] = value;

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

    return;
  }

  void result_cycle(struct timespec& ts, std::stringstream& ss){
    ss << std::scientific;
    ss.precision(6);
    ss << "sec: " << std::setw(8) << ts.tv_sec << " - nsec: " << std::setw(10) << ts.tv_nsec << " --> "
       << std::setw(11) << value << "[s]" << " --> "
       << std::setw(11) << min  << " [s]" << " | "
       << std::setw(11) << mean << " [s]" << " | "
       << std::setw(11) << max  << " [s]" << " { "
       << ((value>DINTERVAL)?("++"):("--")) << " } " << "\n";
  }
};


#endif // MESSUREMENT_H
