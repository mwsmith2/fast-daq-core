#ifndef DAQ_FAST_CORE_SYNC_TRIGGER_HH_    
#define DAQ_FAST_CORE_SYNC_TRIGGER_HH_    

/*==========================================================================*\

file:   sync_trigger.hh
author: Matthias W. Smith

\*==========================================================================*/

//---std includes-----------------------------------------------------------//
#include <string>
#include <thread>
#include <iostream>
#include <list>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <map>

//---other includes---------------------------------------------------------//
#include "zmq.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/property_tree/ptree.hpp"

//---project includes-------------------------------------------------------//
#include "trigger_base.hh"

namespace daq {

class SyncTrigger : TriggerBase {

 public:
  // ctors
  SyncTrigger();
  SyncTrigger(std::string register_address);
  SyncTrigger(std::string register_address, int register_port);
  SyncTrigger(boost::property_tree::ptree &conf);

  // dtor
  ~SyncTrigger() {
    triggering_ = false;
    thread_live_ = false;
    if (trigger_thread_.joinable()) {
      trigger_thread_.join();
    }
    if (register_thread_.joinable()) {
      register_thread_.join();
    }
  }

  void StartTriggers() {triggering_ = true;};
  void StopTriggers() {triggering_ = false;};
  const std::atomic<bool> &clients_good() {return clients_good_;};

  void FixNumClients() {fix_num_clients_ = true;};
  void FixNumClients(int num_clients) { 
    fix_num_clients_ = true;
    num_clients_ = num_clients;
  }
  
 private:

  zmq::context_t ctx_;
  zmq::socket_t trigger_sck_;
  zmq::socket_t register_sck_;
  zmq::socket_t status_sck_;
  zmq::socket_t heartbeat_sck_;

  std::atomic<bool> triggering_;
  std::atomic<bool> fix_num_clients_;

  std::atomic<bool> clients_good_;
  std::atomic<int> num_clients_;
  std::atomic<long> clocks_since_trigger_;

  bool thread_live_;
  std::thread trigger_thread_;
  std::thread register_thread_;

  void TriggerLoop();
  void ClientLoop();

  void DefaultInit();
  void InitSockets();
  void LaunchThreads();
};

} // ::sp

#endif
