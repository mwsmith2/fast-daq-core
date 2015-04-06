#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_CAEN1785_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_CAEN1785_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>
using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"

//--- project includes ------------------------------------------------------//
#include "daq_worker_vme.hh"
#include "daq_structs.hh"

// This class pulls data from a sis_3350 device.
namespace daq {

class DaqWorkerCaen1785 : public DaqWorkerVme<caen_1785> {

public:
  
  // ctor
  DaqWorkerCaen1785(string name, string conf);
  
  void LoadConfig();
  void WorkLoop();
  caen_1785 PopEvent();
  
private:
  
  high_resolution_clock::time_point t0_;
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
