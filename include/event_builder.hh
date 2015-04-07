#ifndef DAQ_FAST_CORE_INCLUDE_EVENT_BUILDER_HH_
#define DAQ_FAST_CORE_INCLUDE_EVENT_BUILDER_HH_

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- projects includes -----------------------------------------------------//
#include "common.hh"
#include "worker_list.hh"
#include "writer_root.hh"

namespace daq {

// This class pulls data form all the workers.
class EventBuilder {

 public:

  // ctor
  EventBuilder(const WorkerList &workers, 
               const std::vector<WriterBase *> writers,
               std::string conf_file);
  
  // dtor
  ~EventBuilder() {
    thread_live_ = false;
    builder_thread_.join();
    push_data_thread_.join();
  }
  
  // member functions
  void StartBuilder() { go_time_ = true; };
  void StopBuilder() { quitting_time_ = true; };
  void LoadConfig();
  bool FinishedRun() { return finished_run_; };
  
private:
  
  // Simple variable declarations
  std::string conf_file_;
  int live_time_;
  int dead_time_;
  long long live_ticks_;
  long long batch_start_;
  int max_event_time_;
  int batch_size_;
  const int kMaxQueueSize = 50;
  
  std::atomic<bool> thread_live_;
  std::atomic<bool> go_time_;
  std::atomic<bool> push_new_data_;
  std::atomic<bool> flush_time_;
  std::atomic<bool> got_last_event_;
  std::atomic<bool> quitting_time_;
  std::atomic<bool> finished_run_;
  
  // Data accumulation variables
  WorkerList workers_;
  std::vector<WriterBase *> writers_;
  std::vector<event_data> push_data_vec_;
  std::queue<event_data> pull_data_que_;
  
  // Concurrency variables
  std::mutex queue_mutex_;
  std::mutex push_data_mutex_;
  std::thread builder_thread_;
  std::thread push_data_thread_;
  
  // Private member functions
  bool WorkersGotSyncEvent();
  void CopyBatch();
  void SendBatch();
  void SendLastBatch();
  void StopWorkers();
  void StartWorkers();

  void BuilderLoop();
  void ControlLoop();
};

} // ::daq

#endif
