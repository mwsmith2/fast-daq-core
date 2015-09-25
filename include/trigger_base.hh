#ifndef DAQ_FAST_CORE_TRIGGER_BASE_HH_    
#define DAQ_FAST_CORE_TRIGGER_BASE_HH_    

/*==========================================================================*\

file:   base_trigger.hh
author: Matthias W. Smith

\*==========================================================================*/

//---std includes-----------------------------------------------------------//
#include <string>

//---other includes---------------------------------------------------------//
#include "zmq.hpp"

//---project includes-------------------------------------------------------//
#include "common.hh"

namespace daq {

class TriggerBase : public CommonBase {

 public:

  TriggerBase(std::string name = "Trigger") : CommonBase(name) {};

 protected:

  const int timeout_ = 100; // in ms
  const long client_timeout_ = 30 * 1e6; // in usec
  const long trigger_timeout_ = 60 * 1e6; // in usec changed from 20 to 60 sec
  const int default_port_ = 42024;
  const std::string default_tcpip_ = "tcp://127.0.0.1";

  int base_port_;
  std::string base_tcpip_;

  std::string trigger_address_;
  std::string register_address_;
  std::string status_address_;
  std::string heartbeat_address_;

  // Convenience function that constructs a full socket address.
  inline std::string ConstructAddress(std::string address, int port) {
    std::string ret = address;
    ret += std::string(":");
    ret += std::to_string(port);
    return ret;
  };
};

} // ::sp

#endif
