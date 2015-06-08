#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1785_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1785_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a sis_3350 device.
namespace daq {

class WorkerCaen1785 : public WorkerVme<caen_1785> {

public:
  
  // ctor
  WorkerCaen1785(std::string name, std::string conf);

  // Read in configuration from a json file.
  // Example file:
  // {
  // 	"device":"/dev/sis1100_00remote",
  // 	"base_address":"0x02000000",
  // 	"read_low_adc":true
  // }
  void LoadConfig();

  // Thread that collects data from the device.
  void WorkLoop();

  // Return the oldest event to the event builder/frontend.
  caen_1785 PopEvent();
  
private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  bool read_low_adc_;
  
  // Ask device if it has an event.
  bool EventAvailable();

  // Clear any extant data from before run.
  void ClearData() { 
    Write16(0x1032, 0x4); 
    Write16(0x1034, 0x4);
  };

  // Read out the event data and add it to the queue.
  void GetEvent(caen_1785 &bundle);

};

} // ::daq

#endif
