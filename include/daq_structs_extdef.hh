#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_EXTDEF_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_EXTDEF_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   daq_struct_extdef.hh

about:  Exists to declare a device handle that all vme classes must share,
        and a mutex to guard it while being defined.

\*===========================================================================*/

#include "daq_structs.hh"

namespace daq {

int vme_dev = -1;
std::string vme_path("/dev/sis1100_00remote");
std::mutex vme_mutex;

bool logging_on = true;
std::string logfile("/home/cenpa/.nmr/daq_log");
std::ofstream logstream(logfile);

int WriteLog(const char *msg) {
  if (logging_on) {
    logstream << msg << std::endl;
    return 0;
  } else {
    return -1;
  }
};

int WriteLog(const std::string& msg) {
  return WriteLog(msg.c_str());
};

} // ::daq

#endif
