#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_DRS4_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_DRS4_HH_

//--- std includes ----------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"
#include "DRS.h"

//--- project includes ------------------------------------------------------//
#include "worker_base.hh"
#include "common.hh"

// This class pulls data from a drs4 evaluation board.
namespace daq {

class WorkerDrs4 : public WorkerBase<drs4> {

public:
  
  // ctor
  WorkerDrs4(std::string name, std::string conf);

  // dtor
  ~WorkerDrs4() {
    // Clear the data.
    go_time_ = false;
    FlushEvents();
    
    // Stop the thread.
    thread_live_ = false;
    if (work_thread_.joinable()) {
      work_thread_.join();
    }

    // free the drs4 object.
    delete drs_;
  }
  
  void LoadConfig();
  void WorkLoop();
  drs4 PopEvent();
  
private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  bool positive_trg_;

  DRS *drs_;
  DRSBoard *board_;
  
  bool EventAvailable();
  void GetEvent(drs4 &bundle);

};

} // ::daq

#endif
