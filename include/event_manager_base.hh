#ifndef DAQ_FAST_CORE_INCLUDE_EVENT_MANAGER_BASE_HH_
#define DAQ_FAST_CORE_INCLUDE_EVENT_MANAGER_BASE_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   event_manager_base.hh
  
  about:  An abstract base class for which different event builders 
          shall descend. It has the basic run control member functions
	  and data organizing functions.

\*===========================================================================*/

//--- std includes ---------------------------------------------------------//
#include <string>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <sys/time.h>

//--- other includes -------------------------------------------------------//

//--- project includes -----------------------------------------------------//
#include "worker_list.hh"
#include "common.hh"

namespace daq {

class EventManagerBase : public CommonBase {
  
public:
  
  // ctor
  EventManagerBase() : CommonBase(std::string("EventManager")) {};
  
  // dtor
  virtual ~EventManagerBase() {};

  // Run control functions to be defined by inheritor.
  virtual int BeginOfRun() = 0;
  virtual int EndOfRun() = 0;
  inline  int PauseRun() { go_time_ = false; };
  inline  int ResumeRun() {go_time_ = true; };

  // Resize event_data to match the proper number of devices in use
  virtual int ResizeEventData(event_data &data) = 0;

  // Returns the oldest stored event.
  inline const event_data &GetCurrentEvent() { return data_queue_.front(); };

  // Removes the oldest event from the front of the queue.
  inline void PopCurrentEvent() {
    queue_mutex_.lock();
    if (!data_queue_.empty()) {
      data_queue_.pop();
    }
    if (data_queue_.empty()) {
      has_event_ = false;
    }
    queue_mutex_.unlock();
  };

  // Called by higher level frontend, i.e., MIDAS.
  inline bool HasEvent() { return has_event_; };

protected:
  
  const int kMaxQueueSize = 10;
  std::string conf_file_;

  std::queue<event_data> data_queue_;
  WorkerList workers_;

  std::atomic<bool> go_time_;
  std::atomic<bool> thread_live_;
  std::atomic<bool> has_event_;
  std::mutex queue_mutex_;
  std::thread run_thread_;

  // Event builder loop that aggregates events and sends them to MIDAS.
  virtual void RunLoop() = 0;
};

} // ::daq

#endif
