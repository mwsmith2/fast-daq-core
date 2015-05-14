#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a caen_1742 device.
namespace daq {

  class WorkerCaen1742 : public WorkerVme<caen_1742> {

public:
  
  // ctor
  WorkerCaen1742(std::string name, std::string conf);

  // dtor
  ~WorkerCaen1742();

  int dev_sn() { return 0;};
  void LoadConfig();
  void WorkLoop();
  caen_1742 PopEvent();

private:

  const float vpp_ = 1.0;
  
  int device_;
  uint size_, bsize_;
  char *buffer_;

  std::chrono::high_resolution_clock::time_point t0_;

  
  bool EventAvailable();
  void GetEvent(caen_1742 &bundle);

};

} // ::daq

#endif
