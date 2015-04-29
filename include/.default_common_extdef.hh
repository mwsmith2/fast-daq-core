#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   common_extdef.hh

about:  Exists to declare a device handle that all vme classes must share,
        and a mutex to guard it while being defined.

\*===========================================================================*/

#include "common.hh"

namespace daq {

int vme_dev = -1;
std::string vme_path("/dev/sis1100_00remote");
std::mutex vme_mutex;

// Set the default logging behavior
bool logging_on = true;
std::string logfile("/usr/local/var/log/fast-daq.log");
std::ofstream logstream(logfile);

// Set the default config directory.
std::string conf_dir("/usr/local/opt/daq/config/");

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
