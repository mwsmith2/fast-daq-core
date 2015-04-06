#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_CAEN6742_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_CAEN6742_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>
using namespace std::chrono;
using std::cout;
using std::cerr;
using std::endl;

//--- other includes --------------------------------------------------------//
#include "CAENDigitizer.h"

//--- project includes ------------------------------------------------------//
#include "daq_worker_vme.hh"
#include "daq_structs.hh"

// This class pulls data from a caen_6742 device.
namespace daq {

class DaqWorkerCaen6742 : public DaqWorkerBase<caen_6742> {

public:
  
  // ctor
  DaqWorkerCaen6742(string name, string conf);

  // dtor
  ~DaqWorkerCaen6742();

  int dev_sn() { return board_info_.SerialNumber; };
  void LoadConfig();
  void WorkLoop();
  caen_6742 PopEvent();

private:

  const float vpp_ = 1.0;
  
  int device_;
  uint size_, bsize_;
  char *buffer_;

  high_resolution_clock::time_point t0_;

  CAEN_DGTZ_BoardInfo_t board_info_;
  CAEN_DGTZ_EventInfo_t event_info_;
  CAEN_DGTZ_X742_EVENT_t *event_;
  
  bool EventAvailable();
  void GetEvent(caen_6742 &bundle);

};

} // ::daq

#endif
