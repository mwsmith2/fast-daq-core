#ifndef DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_SIS3350_HH_
#define DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_SIS3350_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   daq_worker_sis3302.hh
  
  about:  Implements the core functionality of the SIS 3350 device needed
          to operate in this daq.  It loads a config file for several
          settings, then launches a data gathering thread to poll for 
          triggered events.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>

//--- project includes ------------------------------------------------------//
#include "daq_worker_vme.hh"
#include "daq_common.hh"

namespace daq {

// This class pulls data from a sis_3350 device.
class DaqWorkerSis3350 : public DaqWorkerVme<sis_3350> {

public:
  
  // Ctor params:
  // name - used for naming the data branch in ROOT output
  // conf - config file containing parameters like event length 
  //        and sampling rate
  DaqWorkerSis3350(std::string name, std::string conf);

  // Reads the json config file and load the desired parameters.
  // An example:
  // {
  //     "device":vme_path.c_str(),
  //     "base_address":"0x30000000",
  //     "pretrigger_samples":"0x100",
  //     "invert_ext_lemo":true,
  //     "user_led_on":false,
  //     "enable_ext_lemo":true,
  //
  //     "channel_offset":[
  //         -1.7,
  //         -1.7,
  //         -1.7,
  //         -1.7
  //     ],
  //
  //     "channel_gain":[
  //         85,
  //         85,
  //         85,
  //         85
  //     ]
  // }  
  void LoadConfig();

  // The threaded loop that polls for data and pushes events on the queue.
  void WorkLoop();

  // Returns the oldest event on the data queue.
  sis_3350 PopEvent();
  
private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  
  // Checks the device for a triggered event.
  bool EventAvailable();

  // Reads the event data out of the device via vme calls.
  void GetEvent(sis_3350 &bundle);

};

} // ::daq

#endif
