#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_SIS3302_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_SIS3302_HH_

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

class WorkerSis3302 : public WorkerVme<sis_3302> {

 public:
  
  // Ctor params:
  // name - used for naming the data branch in ROOT output
  // conf - config file containing parameters like event length 
  //        and sampling rate
  WorkerSis3302(std::string name, std::string conf);
  
  // Reads the json config file and load the desired parameters.
  // An example:
  // {
  //   "base_address":"0x60000000",
  //   "pretrigger_samples":"0x200",
  //   "user_led_on":false,
  //   "invert_ext_lemo":false,
  //   "enable_int_stop":true,
  //   "enable_ext_lemo":true,
  //   "clock_settings":"3",
  //   "start_delay":"0",
  //   "stop_delay":"0",
  //   "enable_event_length_stop":true
  //   "pretrigger_samples":"0xfff"
  //  }
  void LoadConfig();

  // The threaded loop that polls for data and pushes events on the queue.
  void WorkLoop();

  // Returns the oldest event on the data queue.
  sis_3302 PopEvent();

 private:
  
  // Register constants which are substrings of those given by Struck.
  const static uint CONTROL_STATUS = 0x0;
  const static uint MODID = 0x4;
  const static uint ACQUISITION_CONTROL = 0x10;
  const static uint START_DELAY = 0x14;
  const static uint STOP_DELAY = 0x18;
  const static uint ADC_MEMORY_PAGE = 0x34;
  const static uint EVENT_CONFIG_ADC12 = 0x02000000; 
  const static uint EVENT_CONFIG_ALL_ADC = 0x01000000; 
  const static uint SAMPLE_LENGTH_ALL_ADC = 0x01000004; 
  const static uint PRETRIGGER_DELAY_ALL_ADC = 0x010000060; 
  
  // Key registers.
  const static uint KEY_RESET = 0x400;
  const static uint KEY_ARM = 0x410;
  const static uint KEY_DISARM = 0x414;
  const static uint KEY_START = 0x418;
  const static uint KEY_STOP = 0x41c;
  const static uint KEY_TIMESTAMP_CLR = 0x42C;

  const int kMaxPoll = 500;

  std::chrono::high_resolution_clock::time_point t0_;
  
  // Checks the device for a triggered event.
  bool EventAvailable();

  // Reads the data from the device with vme calls.
  void GetEvent(sis_3302 &bundle);

};

} // ::daq

#endif
