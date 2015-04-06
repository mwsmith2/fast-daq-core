#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_

// Class that covers the basics that DaqWorkers should inherit

//--- std includes ----------------------------------------------------------//
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
using std::string;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- project includes ------------------------------------------------------//

namespace daq {

template <typename T>
class DaqWorkerBase {
  
 public:
  
  //ctor
  DaqWorkerBase(string name, string conf_file) : name_(name),
						 conf_file_(conf_file),
						 thread_live_(true),
						 go_time_(false), 
						 has_event_(false) {};
  
  virtual ~DaqWorkerBase() {
    // Clear the data.
    go_time_ = false;
    FlushEvents();
    
    // Stop the thread.
    thread_live_ = false;
    if (work_thread_.joinable()) {
      work_thread_.join();
    }
  };                                        
  
  // flow control functions
  virtual void StartThread() {
    thread_live_ = true;
    if (work_thread_.joinable()) work_thread_.join();
    work_thread_ = std::thread(&DaqWorkerBase<T>::WorkLoop, this); 
  };
  
  virtual void StopThread() {
    thread_live_ = false;
    usleep(10);
    if (work_thread_.joinable()) work_thread_.join();
  };
  
  void StartWorker() { go_time_ = true; };
  void StopWorker() { go_time_ = false; };
  string name() { return name_; };
  int num_events() { return data_queue_.size(); };
  bool HasEvent() { return has_event_; };
  void FlushEvents() {
    queue_mutex_.lock();
    while (!data_queue_.empty()) {
      data_queue_.pop(); 
    }
    queue_mutex_.unlock();
    has_event_ = false;
  };
  
    // Need to be implented by descendants.
  virtual void LoadConfig() = 0;
    virtual T PopEvent() = 0;
  
 protected:
  
  string name_;
  string conf_file_;
  std::atomic<bool> thread_live_;
  std::atomic<bool> go_time_;
  std::atomic<bool> has_event_;
  std::atomic<int> num_events_;
  
  std::queue<T> data_queue_;
  std::mutex queue_mutex_;
  std::thread work_thread_;
  
  virtual void WorkLoop() = 0;
  
};

} // daq

#endif
