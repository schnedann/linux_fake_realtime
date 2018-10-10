#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <cstddef>

constexpr static bool   const LOG_CYCLES     = true;
constexpr static bool   const RUNTIME_EVAL   = false; //implicit switch of of LOG_CYCLES
                                                      //Data is evaluated after messurement

constexpr static int    const MAX_CYCLES     = 1000;
constexpr static bool   const USE_RT_PRIO    = true; //For Debugging set to false!
//                                            ssmmmuuunnn
constexpr static long   const NSEC_PER_SEC   = 1000000000;
constexpr static size_t const MAX_SAFE_STACK = 8*1024;
//                                            ssmmmuuunnn
constexpr static size_t const INTERVAL       =    5000000; //5ms
constexpr static double const DINTERVAL      = static_cast<double>(INTERVAL)/static_cast<double>(NSEC_PER_SEC);

#endif // CONFIG_H
