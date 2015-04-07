#ifndef DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_CAEN1785_HH_
#define DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_CAEN1785_HH_

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

class DaqWorkerCaen1785 : public DaqWorkerVme<caen_1785> {

public:
  
  // ctor
  DaqWorkerCaen1785(std::string name, std::string conf);
  
  void LoadConfig();
  void WorkLoop();
  caen_1785 PopEvent();
  
private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  bool read_low_adc_;
  
  bool EventAvailable();
  void ClearData() { 
    Write16(0x1032, 0x4); 
    Write16(0x1034, 0x4);
  };
  void GetEvent(caen_1785 &bundle);

};

} // ::daq

#endif
