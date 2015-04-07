#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_FAKE_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <cmath>
#include <chrono>
#include <ctime>
#include <thread>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "worker_base.hh"
#include "common.hh"


// This class produces fake data to test functionality
namespace daq {

typedef sis_3350 event_struct;

class WorkerFake : public WorkerBase<event_struct> {
  
 public:
  
  // ctor
  WorkerFake(std::string name, std::string conf);
  
  ~WorkerFake() {
    thread_live_ = false;
    if (event_thread_.joinable()) event_thread_.join();
  };
  
  void StartThread() {
    thread_live_ = true;
    if (work_thread_.joinable()) work_thread_.join();
    if (event_thread_.joinable()) event_thread_.join();
    work_thread_ = std::thread(&WorkerFake::WorkLoop, this);
    event_thread_ = std::thread(&WorkerFake::GenerateEvent, this);
  };
  
  void StopThread() {
    thread_live_ = false;
    if (work_thread_.joinable()) work_thread_.join();
    if (event_thread_.joinable()) event_thread_.join();
  };
  
  void LoadConfig();
  void WorkLoop();
  event_struct PopEvent();
  
 private:
  
  // Fake data variables
  int num_ch_;
  int len_tr_;
  std::atomic<bool> has_fake_event_;
  double rate_;
  double jitter_;
  double drop_rate_;
  std::chrono::high_resolution_clock::time_point t0_;
  
  // Concurrent data generation.
  event_struct event_data_;
  std::thread event_thread_;
  std::mutex event_mutex_;
  
  bool EventAvailable() { return has_fake_event_; };
  void GetEvent(event_struct &bundle);

  // The function generates fake data.
  void GenerateEvent();
};
  
} // ::daq

#endif
