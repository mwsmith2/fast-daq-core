#ifndef DAQ_FAST_CORE_TRIGGER_BASE_HH_    
#define DAQ_FAST_CORE_TRIGGER_BASE_HH_    

/*==========================================================================*\

file:   base_trigger.hh
author: Matthias W. Smith

\*==========================================================================*/

//---std includes-----------------------------------------------------------//
#include <string>
#include <sys/time.h>

//---other includes---------------------------------------------------------//
#include "zmq.hpp"

//---project includes-------------------------------------------------------//
#include "common.hh"

namespace daq {

inline void light_sleep() {
  usleep(100); // in usec
}

inline void heavy_sleep() {
  usleep(100000); // in usec
}

inline long systime_us() {
  static timeval t;
  gettimeofday(&t, nullptr);
  return 1000000*t.tv_sec + t.tv_usec;
}

class TriggerBase {

 public:

 protected:

  const int timeout_ = 100; // in ms
  const long client_timeout_ = 1 * 1e6; // in usec
  const long trigger_timeout_ = 20 * 1e6; // in usec
  const int default_port_ = 42024;
  const std::string default_tcpip_ = "tcp://127.0.0.1";

  int base_port_;
  std::string base_tcpip_;

  std::string trigger_address_;
  std::string register_address_;
  std::string status_address_;
  std::string heartbeat_address_;

  inline std::string ConstructAddress(std::string address, int port) {
    std::string ret = address;
    ret += std::string(":");
    ret += std::to_string(port);
    return ret;
  };
};

} // ::sp

#endif
