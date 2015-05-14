#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   common_extdef.hh

about:  Exists to instantiate some of the shared variables for the daq.

\*===========================================================================*/

#include "common.hh"

namespace daq {

int vme_dev = -1;
std::string vme_path("/dev/sis1100_00remote");
std::mutex vme_mutex;

// Set the default config directory.
std::string conf_dir("/usr/local/opt/daq/config/");

// Set up a global zmq context
zmq::context_t msg_context(1);

// Set the default logging behavior
bool logging_on = true;
std::string logfile("/usr/local/var/log/daq/fast-daq.log");
std::ofstream logstream(logfile);

int WriteLog(const char *format, ...) {
  static char str[512];
  
  if (logging_on) {
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);

    logstream << str << std::endl;
    return 0;
    
  } else {
    
    return -1;
  }
};

int WriteLog(const std::string &format, ...) {
  static char str[512];
  
  if (logging_on) {
    va_list args;
    va_start(args, format.c_str());
    vsprintf(str, format.c_str(), args);
    va_end(args);

    logstream << str << std::endl;
    return 0;
    
  } else {
    
    return -1;
  }
};

} // ::daq

#endif
