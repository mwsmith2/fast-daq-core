#ifndef DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_SIS3302_HH_
#define DAQ_FAST_CORE_INCLUDE_DAQ_WORKER_SIS3302_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   worker_sis3302.hh
  
  about:  Implements the core functionality of the SIS 3302 device needed
          to operate in this daq.  It loads a config file for several
          settings, then launches a data gathering thread to poll for 
          triggered events.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a sis_3302 device.
namespace daq {

class DaqWorkerSis3302 : public DaqWorkerVme<sis_3302> {

 public:
  
  // Ctor params:
  // name - used for naming the data branch in ROOT output
  // conf - config file containing parameters like event length 
  //        and sampling rate
  DaqWorkerSis3302(std::string name, std::string conf);
  
  // Reads the json config file and load the desired parameters.
  // An example:
  // {
  //   "device":vme_path.c_str(),
  //   "base_address":"0x60000000",
  //   "user_led_on":false,
  //   "invert_ext_lemo":false,
  //   "enable_ext_lemo":true,
  //   "clock_settings":"3",
  //   "start_delay":"0",
  //   "stop_delay":"0",
  //   "enable_event_length_stop":true,
  //   "pretrigger_samples":"0xfff"
  //  }
  void LoadConfig();

  // The threaded loop that polls for data and pushes events on the queue.
  void WorkLoop();

  // Returns the oldest event on the data queue.
  sis_3302 PopEvent();

 private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  
  // Checks the device for a triggered event.
  bool EventAvailable();

  // Reads the data from the device with vme calls.
  void GetEvent(sis_3302 &bundle);

};

} // ::daq

#endif
