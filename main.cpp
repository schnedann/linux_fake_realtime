/**
 * Found the basic code in various places in different maturity levels
 * cleaned it up, reconstructed missing parts, added some usefull output & messurements,
 * Code seems to work quite well in providing a stable cycle,
 * even browsing in parallel has no effect on the messurements.
 *
 * Running code on raspberryPi pending
 *
 * Danny Schneider, 2018
 */

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sched.h>

using namespace std;

constexpr static int    const MAX_CYCLES     = 1000000;
constexpr static long   const NSEC_PER_SEC   = 1000000000;
constexpr static size_t const MAX_SAFE_STACK = 8*1024;
constexpr static size_t const INTERVAL       = 50000; //0.05ms = 50µs

/**
 * Not understanding why this piece of code
 * should reserve stack... as dummy only lives
 * in the scope of stack_prefault()
 *
 * --> assume Code has no effect at last
 */
void stack_prefault(){
  unsigned char dummy[MAX_SAFE_STACK];
  memset(&dummy, 0, MAX_SAFE_STACK);
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

void do_work(struct timespec& ts){
  //omit first run
  static bool valid = false;

  //Calculate Cycle Time
  static long oldns = 0;
  long difftime = ts.tv_nsec-oldns;
  if(difftime<0) difftime += 1000000000;

  //Monitor Min and Max
  static long min = long(0x7FFFFFFFul);
  static long max = 0;
  if(valid && difftime<min) min = difftime;
  if(valid && difftime>max) max = difftime;

  //Calculate Mean Cycle Time
  static long mean = 0;
  mean = mean + ((difftime - mean)>>1);

  //Output...
  cout << "sec: " << setw(8) << ts.tv_sec << " - nsec: " << setw(10) << ts.tv_nsec << " --> " << setw(8) << difftime << " --> " << setw(8) << min <<" | " << setw(8) << mean <<" | " << setw(8) << max <<"\n";

  //Prepare next cycle
  oldns = ts.tv_nsec;
  valid |= true;
  return;
}

int main()
{
  int err = 0;
  cout << "Hello Linux Fake Realtime...!" << endl;

  uint32_t ccnt=0; //Cycle Counter

  struct timespec t;
  struct sched_param param;
  /* Declare ourself as a "real time" task */
  { //set to 90% of max Priority
    int const pmin = sched_get_priority_min(SCHED_FIFO);
    int const pmax = sched_get_priority_max(SCHED_FIFO);
    int const pdiff = ((pmax-pmin)*900000)/1000000;
    param.sched_priority = pmin+pdiff;
  }
  if(sched_setscheduler(0, SCHED_FIFO, &param) == -1){
    err=-1;
    goto lERR;
  }
  /* Lock memory */
  if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1){
    err=-2;
    goto lERR;
  }
  /* Stack reservieren */
  stack_prefault();                   //?
  clock_gettime(CLOCK_MONOTONIC, &t); //Guess CLOCK_PROCESS_CPUTIME_ID should work alike...
  while(1) {
    /* Arbeit durchführen */
    {
      do_work(t);
      ++ccnt;
      if(ccnt>MAX_CYCLES) break;
    }
    t.tv_nsec += INTERVAL; //Set next Wakeup Time
    tsnorm(t);             //Overflow handling
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
  }

lERR:
  if(err) cout << "Err: " << err << "\n";
  return err;
}
