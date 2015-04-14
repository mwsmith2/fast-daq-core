#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   common.hh

about:  Contains the data structures for several hardware devices in a single
        location.  The header should be included in any program that aims
        to interface with (read or write) data with this daq.

\*===========================================================================*/

#define SIS_3350_CH 4
#define SIS_3350_LN 1024
#define SIS_3302_CH 8 
#define SIS_3302_LN 100000

#define CAEN_1785_CH 8

#define CAEN_6742_GR 2
#define CAEN_6742_CH 18
#define CAEN_6742_LN 1024

#define DRS4_CH 4 
#define DRS4_LN 1024

// NMR specific stuff
#define SHORT_FID_LN SIS_3302_LN / 10
#define NMR_FID_LN SIS_3302_LN
#define SHIM_PLATFORM_CH 25
#define SHIM_UW_TEST_CH 25
#define SHIM_FIXED_CH 4
#define RUN_TROLLEY_CH 17
#define RUN_FIXED_CH 378

//--- std includes ----------------------------------------------------------//
#include <vector>
#include <mutex>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include "TFile.h"

//--- projects includes -----------------------------------------------------//
#include "worker_base.hh"

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

// A macro to define nmr structs since they are very similar.
#define MAKE_NMR_STRUCT(name, num_ch, len_tr)\
struct name {\
  ULong64_t clock_sec[num_ch];\
  ULong64_t clock_usec[num_ch];\
  Double_t fid_time[num_ch];\
  Double_t snr[num_ch];\
  Double_t freq[num_ch];\
  Double_t freq_err[num_ch];\
  UShort_t  trace[num_ch][len_tr];\
};\

MAKE_NMR_STRUCT(shim_platform, SHIM_PLATFORM_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(shim_platform_st, SHIM_PLATFORM_CH, SHORT_FID_LN);
MAKE_NMR_STRUCT(shim_fixed, SHIM_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(shim_fixed_st, SHIM_FIXED_CH, SHORT_FID_LN);

MAKE_NMR_STRUCT(run_trolley, RUN_TROLLEY_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(run_trolley_st, RUN_TROLLEY_CH, SHORT_FID_LN);
MAKE_NMR_STRUCT(run_fixed, RUN_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(run_fixed_st, RUN_FIXED_CH, SHORT_FID_LN);

// flexible struct built from basic structs.
// Built from basic structs 
struct event_data {
  std::vector<sis_3350> sis_fast;
  std::vector<sis_3302> sis_slow;
  std::vector<caen_1785> caen_adc;
  std::vector<caen_6742> caen_drs;
  std::vector<drs4> drs;
};

struct nmr_data {
  std::vector<ULong64_t> clock_sec;
  std::vector<ULong64_t> clock_usec;
  std::vector<Double_t> fid_time;
  std::vector<Double_t> snr;
  std::vector<Double_t> freq;
  std::vector<Double_t> freq_err;
  std::vector< std::array<UShort_t, NMR_FID_LN> > trace;

  inline void Resize(int size) {
    clock_sec.resize(size);
    clock_usec.resize(size);
    fid_time.resize(size);
    snr.resize(size);
    freq.resize(size);
    freq_err.resize(size);
    trace.resize(size);
  }
};
  
// Typedef for all workers - needed by in WorkerList
typedef boost::variant<WorkerBase<sis_3350> *, 
                       WorkerBase<sis_3302> *, 
                       WorkerBase<caen_1785> *, 
                       WorkerBase<caen_6742> *,
                       WorkerBase<drs4> *> 
worker_ptr_types;

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
const int short_sleep = 10;
const int long_sleep = 100;
const int sample_period = 0.1; // in microseconds

} // ::daq

#endif
