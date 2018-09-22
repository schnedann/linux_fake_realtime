/**
 * Found the basic code in various places in different maturity levels
 * cleaned it up, reconstructed missing parts, added some usefull output & messurements,
 * Code seems to work quite well in providing a stable cycle,
 * even browsing in parallel has no effect on the messurements.
 *
 * Danny Schneider, 2018
 */
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <array>
#include <sstream>
#include <ctime>
#include <chrono>
#include <cstring>
#include <limits>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sched.h>

using namespace std;

constexpr static int    const MAX_CYCLES     = 1000;
//                                            ssmmmuuunnn
constexpr static long   const NSEC_PER_SEC   = 1000000000;
constexpr static size_t const MAX_SAFE_STACK = 8*1024;
//                                            ssmmmuuunnn
constexpr static size_t const INTERVAL       =    5000000; //5ms
constexpr static double const DINTERVAL      = static_cast<double>(INTERVAL)/static_cast<double>(NSEC_PER_SEC);
//-----

/**
 * Allocating Memory Once causes Stack to grow
 * if Later Mamory is needed, the OS can use the
 * allready large Stack without causing Pagefaults
 */
void stack_prefault(){
  volatile std::array<char,MAX_SAFE_STACK> dummy{};
  /*unsigned char dummy[MAX_SAFE_STACK];
  memset(&dummy, 0, MAX_SAFE_STACK);*/
  return;
}

/**
 * correct time-struct on nanosecond overflow
 */
static void tsnorm(struct timespec& ts){
  while (ts.tv_nsec >= NSEC_PER_SEC) {
    ts.tv_nsec -= NSEC_PER_SEC;
    ++ts.tv_sec;
  }
  return;
}

void do_work(struct timespec& ts, std::stringstream& ss){
  //omit first run
  static uint8_t valid = 0;

  //Messure Precision with stdlib
  static std::chrono::high_resolution_clock::time_point hrt1 = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point hrt2 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(hrt2 - hrt1);
  hrt1 = hrt2; //Prepare next cycle

  //Monitor Min and Max
  static double min = std::numeric_limits<double>::max();
  static double max = 0;
  double const value = time_span.count();
  if((valid>=10) && (value<min)){
    min = value;
  }
  if((valid>=10) && (value>max)){
    max = value;
  }

  //Calculate Mean Cycle Time
  static double mean = 0;
  if(valid>=10){
    mean = mean + ((value - mean)/2);
  }

  //Output...
  ss << scientific;
  ss.precision(6);
  ss << "sec: " << setw(8) << ts.tv_sec << " - nsec: " << setw(10) << ts.tv_nsec << " --> "
     << setw(11) << value << "[s]" << " --> "
     << setw(11) << min  << " [s]" << " | "
     << setw(11) << mean << " [s]" << " | "
     << setw(11) << max  << " [s]" << " { "
     << ((value>DINTERVAL)?("++"):("--")) << " } " << "\n";

  //Prepare next cycle
  if(valid<10)++valid;
  return;
}

int main(){
  int err = 0;
  cout << "Hello Linux Fake Realtime...!" << endl;

  uint32_t ccnt=0; //Cycle Counter
  std::stringstream ss;

  struct timespec t;
  struct sched_param param;
  /* Declare ourself as a "real time" task */
  { //set to 90% of max Priority
    int const pmin = sched_get_priority_min(SCHED_FIFO);
    int const pmax = sched_get_priority_max(SCHED_FIFO);
    int const pdiff = ((pmax-pmin)*95000000)/100000000;
    param.sched_priority = pmin+pdiff;
  }
  if(sched_setscheduler(0, SCHED_FIFO, &param) == -1){
    err=-1;
    goto lERR;
  }
  /* Lock memory (do not swap!)*/
  if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1){
    err=-2;
    goto lERR;
  }
  stack_prefault();                   //Stack reservieren
  clock_gettime(CLOCK_MONOTONIC, &t); //Guess CLOCK_PROCESS_CPUTIME_ID should work alike...
  while(1) {
    /* Arbeit durchführen */
    {
      do_work(t,ss);
      ++ccnt;
      if(ccnt>MAX_CYCLES) break;
    }
    t.tv_nsec += INTERVAL; //Set next Wakeup Time
    tsnorm(t);             //Overflow handling
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
  }

  cout << ss.str() << "\n";

lERR:
  if(err) cout << "Err: " << err << "\n";
  return err;
}
