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

class SyncClient : public TriggerBase {

 public:

  // ctor
  SyncClient();
  SyncClient(std::string address);
  SyncClient(std::string address, int port);
  SyncClient(std::string name, std::string address);
  SyncClient(std::string name, std::string address, int port);

  ~SyncClient() {
    thread_live_ = false;

    if (status_thread_.joinable()) {
      status_thread_.join();
    }

    if (restart_thread_.joinable()) {
      restart_thread_.join();
    }

    if (heartbeat_thread_.joinable()) {
      heartbeat_thread_.join();
    }
  }

  // Getters
  const std::atomic<bool> &ready() {return ready_;};
  const std::atomic<int> &trigger_counter() {return trigger_counter_;};

  // Setters
  void UnsetReady() {
    ready_ = false;
    got_trigger_ = false;
    sent_ready_ = false;
    trigger_counter_ = 0;
  };
  void SetReady() {ready_ = true;};
  bool HasTrigger();

 private:

  std::atomic<bool> connected_;
  std::atomic<bool> ready_;
  std::atomic<bool> sent_ready_;
  std::atomic<bool> got_trigger_;
  std::atomic<bool> thread_live_;

  std::atomic<int> trigger_counter_;

  std::string client_name_;

  std::thread status_thread_;
  std::thread restart_thread_;
  std::thread heartbeat_thread_;

  zmq::socket_t register_sck_;
  zmq::socket_t trigger_sck_;
  zmq::socket_t status_sck_;
  zmq::socket_t heartbeat_sck_;

  // Read configuration file and launch different threads.
  void DefaultInit();

  // Initialized the sockets according the config file, called by DefaultInit.
  void InitSockets();

  // Launch the socket threads, called by DefaultInit.
  void LaunchThreads();

  // Thread that handles synchronizing triggers.
  void StatusLoop();

  // Thread that handles restarts if we break connection.
  void RestartLoop();

  // Communicates with clients.
  void HeartbeatLoop();
};

} // ::daq

#endif
