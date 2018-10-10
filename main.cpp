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
#include <algorithm>
#include <functional>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sched.h>

#include "config.h"
#include "messurement.h"

using namespace std;

//-----

/**
 * @brief stack_prefault - Allocating Memory Once causes Stack to grow
 *                         if Later Mamory is needed, the OS can use the
 *                         allready large Stack without causing Pagefaults
 * @return
 */
bool stack_prefault(){
  //null initialized Array
  std::array<char,MAX_SAFE_STACK> dummy{};
  //"use" variable :-) --> compiler warning
  bool res = true;
  std::function<void(char const&)> fkt = [&res](char const& _x){
    res &= (_x==0);
    return;
  };
  std::for_each(dummy.begin(), dummy.end(), fkt);
  return res;
}

/**
 * @brief tsnorm - correct time-struct on nanosecond overflow
 * @param ts
 */
static void tsnorm(struct timespec& ts){
  while (ts.tv_nsec >= NSEC_PER_SEC) {
    ts.tv_nsec -= NSEC_PER_SEC;
    ++ts.tv_sec;
  }
  return;
}

/**
 * @brief do_work
 * @param ts
 * @param ss
 */
void do_work(struct timespec& ts, std::stringstream& ss){
  static messurement<MAX_CYCLES> mm;
  mm.take();

  //Output...
  if(LOG_CYCLES){
    mm.result_cycle(ts,ss);
  }
}

/**
 * @brief main
 * @return
 */
int main(){
  int err = 0;
  cout << "Hello Linux Fake Realtime...!" << endl;

  uint32_t ccnt=0; //Cycle Counter
  std::stringstream ss;

  struct timespec t;

  if(USE_RT_PRIO){
    /* Declare ourself as a "real time" task */
    { //set to 90% of max Priority
      struct sched_param param;

      int const pmin = sched_get_priority_min(SCHED_FIFO);
      int const pmax = sched_get_priority_max(SCHED_FIFO);
      int const pdiff = ((pmax-pmin)*90000000)/100000000;
      param.sched_priority = pmin+pdiff;

      if(sched_setscheduler(0, SCHED_FIFO, &param) == -1){
        err=-1;
        goto lERR;
      }
    }
    /* Lock memory (do not swap!)*/
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1){
      err=-2;
      goto lERR;
    }
    /* Stack reservieren*/
    if(!stack_prefault()){
      err=-3;
      goto lERR;
    }
  }

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
