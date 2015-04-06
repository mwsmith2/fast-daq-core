#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_DRS4_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_DRS4_HH_

//--- std includes ----------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <chrono>
#include <iostream>
using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"
#include "DRS.h"

//--- project includes ------------------------------------------------------//
#include "daq_worker_vme.hh"
#include "daq_structs.hh"

// This class pulls data from a drs4 evaluation board.
namespace daq {

class DaqWorkerDrs4 : public DaqWorkerBase<drs4> {

public:
  
  // ctor
  DaqWorkerDrs4(string name, string conf);

  // dtor
  ~DaqWorkerDrs4() {
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
  
  high_resolution_clock::time_point t0_;
  bool positive_trg_;

  DRS *drs_;
  DRSBoard *board_;
  
  bool EventAvailable();
  void GetEvent(drs4 &bundle);

};

} // ::daq

#endif
