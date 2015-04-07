#ifndef DAQ_FAST_CORE_SYNC_CLIENT_HH_    
#define DAQ_FAST_CORE_SYNC_CLIENT_HH_    

/*==========================================================================*\

file:   sync_client.hh
author: Matthias W. Smith

\*==========================================================================*/

//---std includes-----------------------------------------------------------//
#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <ctime>

//---other includes---------------------------------------------------------//
#include "zmq.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/uuid/uuid_io.hpp"

//---project includes-------------------------------------------------------//
#include "trigger_base.hh"

namespace daq {

class SyncClient : TriggerBase {

 public:

  // ctor
  SyncClient();
  SyncClient(std::string address);
  SyncClient(std::string address, int port);

  ~SyncClient() {
    thread_live_ = false;

    if (status_thread_.joinable()) {
      status_thread_.join();
    }

    if (restart_thread_.joinable()) {
      restart_thread_.join();
    }
  }

  // Getters
  const std::atomic<bool> &ready() {return ready_;};

  // Setters
  void UnsetReady() {ready_ = false;};
  void SetReady() {ready_ = true;};
  bool HasTrigger();

 private:

  std::atomic<bool> connected_;
  std::atomic<bool> ready_;
  std::atomic<bool> sent_ready_;
  std::atomic<bool> got_trigger_;
  std::atomic<bool> thread_live_;

  std::string client_name_;

  std::thread status_thread_;
  std::thread restart_thread_;
  std::thread heartbeat_thread_;

  zmq::context_t ctx_;
  zmq::socket_t register_sck_;
  zmq::socket_t trigger_sck_;
  zmq::socket_t status_sck_;
  zmq::socket_t heartbeat_sck_;

  void DefaultInit();
  void InitSockets();
  void LaunchThread();
  void StatusLoop();
  void RestartLoop();
  void HeartbeatLoop();
};

} // ::sp

#endif
