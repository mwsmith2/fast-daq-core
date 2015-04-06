#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3302_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3302_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>
using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "daq_worker_vme.hh"
#include "daq_structs.hh"

// This class pulls data from a sis_3302 device.
namespace daq {

class DaqWorkerSis3302 : public DaqWorkerVme<sis_3302> {

public:
  
  // ctor
  DaqWorkerSis3302(string name, string conf);
  
  void LoadConfig();
  void WorkLoop();
  sis_3302 PopEvent();

private:
  
  high_resolution_clock::time_point t0_;
  
  bool EventAvailable();
  void GetEvent(sis_3302 &bundle);

};

} // ::daq

#endif
