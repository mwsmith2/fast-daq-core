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

#define CAEN_1742_GR 4
#define CAEN_1742_CH 32
#define CAEN_1742_LN 1024

#define DRS4_CH 4 
#define DRS4_LN 1024

#define TEK_SCOPE_CH 4
#define TEK_SCOPE_LN 10000

// NMR specific stuff
#define NMR_FID_LN SIS_3302_LN
#define SHORT_FID_LN 10000
#define SHIM_PLATFORM_CH 25
#define SHIM_UW_TEST_CH 25
#define SHIM_FIXED_CH 4
#define RUN_TROLLEY_CH 17
#define RUN_FIXED_CH 378

//--- std includes ----------------------------------------------------------//
#include <vector>
#include <mutex>
#include <cstdarg>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include <zmq.hpp>
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

struct caen_1742 {
  ULong64_t system_clock;
  ULong64_t device_clock[CAEN_1742_CH];
  UShort_t trace[CAEN_1742_CH][CAEN_1742_LN];
  UShort_t trigger[CAEN_1742_GR][CAEN_1742_LN];
};

struct drs4 {
  ULong64_t system_clock;
  ULong64_t device_clock[DRS4_CH];
  UShort_t trace[DRS4_CH][DRS4_LN];
};

struct tek_scope {
  ULong64_t system_clock;
  ULong64_t device_clock[TEK_SCOPE_CH];
  Double_t time[TEK_SCOPE_CH][TEK_SCOPE_LN];
  Double_t trace[TEK_SCOPE_CH][TEK_SCOPE_LN];
};

// Built from basic structs 
struct event_data {
  std::vector<sis_3350> sis_fast;
  std::vector<sis_3302> sis_slow;
  std::vector<caen_1785> caen_adc;
  std::vector<caen_6742> caen_drs;
  std::vector<caen_1742> caen_1742_vec;
  std::vector<drs4> drs;
};

// NMR specific stuff
// A macro to define nmr structs since they are very similar.
#define MAKE_NMR_STRUCT(name, num_ch, len_tr)\
struct name {\
  Double_t sys_clock[num_ch];\
  Double_t gps_clock[num_ch];\
  Double_t dev_clock[num_ch];\
  Double_t snr[num_ch];\
  Double_t len[num_ch];\
  Double_t freq[num_ch];\
  Double_t ferr[num_ch];\
  Double_t freq_zc[num_ch];\
  Double_t ferr_zc[num_ch];\
  UShort_t health[num_ch];\
  UShort_t method[num_ch];\
  UShort_t trace[num_ch][len_tr];\
};

// Might as well define a root branch string for the struct.
#define NMR_HELPER(name, num_ch, len_tr) \
const char * const name = "sys_clock["#num_ch"]/D:gps_clock["#num_ch"]/D:"\
"dev_clock["#num_ch"]/D:snr["#num_ch"]/D:len["#num_ch"]/D:freq["#num_ch"]/D:"\
"ferr["#num_ch"]/D:freq_zc["#num_ch"]/D:ferr_zc["#num_ch"]/D:"\
"health["#num_ch"]/s:method["#num_ch"]/s:trace["#num_ch"]["#len_tr"]/s"

#define MAKE_NMR_STRING(name, num_ch, len_tr) NMR_HELPER(name, num_ch, len_tr)


MAKE_NMR_STRUCT(shim_platform, SHIM_PLATFORM_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(shim_platform_st, SHIM_PLATFORM_CH, SHORT_FID_LN);
MAKE_NMR_STRUCT(shim_fixed, SHIM_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(shim_fixed_st, SHIM_FIXED_CH, SHORT_FID_LN);

MAKE_NMR_STRUCT(run_trolley, RUN_TROLLEY_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(run_trolley_st, RUN_TROLLEY_CH, SHORT_FID_LN);
MAKE_NMR_STRUCT(run_fixed, RUN_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRUCT(run_fixed_st, RUN_FIXED_CH, SHORT_FID_LN);

MAKE_NMR_STRING(shim_platform_string, SHIM_PLATFORM_CH, NMR_FID_LN);
MAKE_NMR_STRING(shim_platform_st_string, SHIM_PLATFORM_CH, SHORT_FID_LN);
MAKE_NMR_STRING(shim_fixed_string, SHIM_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRING(shim_fixed_st_string, SHIM_FIXED_CH, SHORT_FID_LN);

MAKE_NMR_STRING(run_trolley_string, RUN_TROLLEY_CH, NMR_FID_LN);
MAKE_NMR_STRING(run_trolley_st_string, RUN_TROLLEY_CH, SHORT_FID_LN);
MAKE_NMR_STRING(run_fixed_string, RUN_FIXED_CH, NMR_FID_LN);
MAKE_NMR_STRING(run_fixed_st_string, RUN_FIXED_CH, SHORT_FID_LN);

// flexible struct built from the basic nmr attributes.
struct nmr_data {
  std::vector<Double_t> sys_clock;
  std::vector<Double_t> gps_clock;
  std::vector<Double_t> dev_clock;
  std::vector<Double_t> snr;
  std::vector<Double_t> len;
  std::vector<Double_t> freq;
  std::vector<Double_t> ferr;
  std::vector<Double_t> freq_zc;
  std::vector<Double_t> ferr_zc;
  std::vector<UShort_t> health;
  std::vector<UShort_t> method;
  std::vector< std::array<UShort_t, NMR_FID_LN> > trace;

  inline void Resize(int size) {
    sys_clock.resize(size);
    gps_clock.resize(size);
    dev_clock.resize(size);
    snr.resize(size);
    len.resize(size);
    freq.resize(size);
    ferr.resize(size);
    freq_zc.resize(size);
    ferr_zc.resize(size);
    health.resize(size);
    method.resize(size);
    trace.resize(size);
  }
};
  
// Typedef for all workers - needed by in WorkerList
typedef boost::variant<WorkerBase<sis_3350> *, 
                       WorkerBase<sis_3302> *, 
                       WorkerBase<caen_1785> *, 
                       WorkerBase<caen_6742> *,
                       WorkerBase<drs4> *,
                       WorkerBase<caen_1742> *>
worker_ptr_types;

// A useful define guard for I/O with the vme bus.
extern int vme_dev;
extern std::string vme_path;
extern std::mutex vme_mutex;

// Create a variable for a config directory.
extern std::string conf_dir;

// Set sleep times for data polling threads.
const int short_sleep = 10;
const int long_sleep = 100;
const int sample_period = 0.1; // in microseconds

// Set up a global zmq context
extern zmq::context_t msg_context;

} // ::daq

#endif
