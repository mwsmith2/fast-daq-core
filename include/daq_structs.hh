#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   daq_common.hh

about:  Contains the data structures for several hardware devices in a single
        location.  The header should be included in any program that aims
        to interface with (read or write) data with this daq.

\*===========================================================================*/


#define SIS_3350_CH 4
//#define SIS_3350_LN 1024
#define SIS_3350_LN 0x40000 // ~250,000

#define SIS_3302_CH 8 
#define SIS_3302_LN 1024

#define CAEN_1785_CH 8

#define CAEN_6742_GR 2
#define CAEN_6742_CH 18
#define CAEN_6742_LN 1024

#define DRS4_CH 4 
#define DRS4_LN 1024

//--- std includes ----------------------------------------------------------//
#include <mutex>
#include <vector>
using std::vector;

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include "TFile.h"

//--- projects includes -----------------------------------------------------//
#include "daq_worker_base.hh"

namespace daq {

// Basic structs
struct sis_3350 {
  ULong64_t system_clock;
  ULong64_t device_clock[SIS_3350_CH];
  UShort_t trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 {
  ULong64_t system_clock;
  ULong64_t device_clock[SIS_3302_CH];
  UShort_t trace[SIS_3302_CH][SIS_3302_LN];
};

struct caen_1785 {
  ULong64_t system_clock;
  ULong64_t device_clock[CAEN_1785_CH];
  UShort_t value[CAEN_1785_CH];
};

struct caen_6742 {
  ULong64_t system_clock;
  ULong64_t device_clock[CAEN_6742_CH];
  UShort_t trace[CAEN_6742_CH][CAEN_6742_LN];
};

struct drs4 {
  ULong64_t system_clock;
  ULong64_t device_clock[DRS4_CH];
  UShort_t trace[DRS4_CH][DRS4_LN];
};

// Built from basic structs 
struct event_data {
  vector<sis_3350> sis_fast;
  vector<sis_3302> sis_slow;
  vector<caen_1785> caen_adc;
  vector<caen_6742> caen_drs;
  vector<drs4> drs;
};

// typedef for all workers
typedef boost::variant<DaqWorkerBase<sis_3350> *, 
                       DaqWorkerBase<sis_3302> *, 
                       DaqWorkerBase<caen_1785> *, 
                       DaqWorkerBase<caen_6742> *,
                       DaqWorkerBase<drs4> *> worker_ptr_types;

// A useful define guard for I/O with the vme bus.
extern int vme_dev;
extern std::string vme_path;
extern std::mutex vme_mutex;

// Create a single log file for the whole DAQ.
extern bool logging_on; 
extern std::string logfile;
extern std::ofstream logstream;

int WriteLog(const char *msg);
int WriteLog(const std::string& msg);

// Set sleep times for data polling threads.
const int kShortSleep = 20;
const int kLongSleep = 200;
const int kMinEventTime = 100; // in microseconds

} // ::daq

#endif
