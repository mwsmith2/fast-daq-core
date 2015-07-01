#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_SIS3316_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_SIS3316_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   worker_sis3316.hh
  
  about:  Implements the core functionality of the SIS 3316 device needed
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

// This class pulls data from a sis_3316 device.
namespace daq {

class WorkerSis3316 : public WorkerVme<sis_3316> {

 public:
  
  // Ctor params:
  // name - used for naming the data branch in ROOT output
  // conf - config file containing parameters like event length 
  //        and sampling rate
  WorkerSis3316(std::string name, std::string conf);
  
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
  sis_3316 PopEvent();

 private:

  // Registers
  const static uint kRegDevBase = 0x0000;
  const static uint kRegDevInfo = 0x0004;
  const static uint kRegDevHwRev = 0x001c;
  const static uint kRegDevTemp = 0x0020;
  const static uint kRegAcqStatus = 0x0060;
  const static uint kRegTapDelay = 0x1000;
  const static uint kRegDacOffset = 0x1008;
  const static uint kRegSpiCtrl = 0x100c;
  const static uint kRegAdcHeader = 0x1014;

  const static uint kKeyDevReset = 0x400;
  const static uint kKeyDevDisarm = 0x414;
  const static uint kKeyIntTrigger = 0x418;

  // Magic/useful constants
  const static uint kAdcGroupOffset = 0x1000;
  const static uint kAdcChanOffset = 0x4;
  const static uint kAdcTapCalib = 0xf00;
  const static uint kAdcEnableBit = 0x1 << 24;
  const static uint kDacEnableRef = 0x88f00001;

  // Variables
  std::chrono::high_resolution_clock::time_point t0_;
  std::atomic<bool> bank2_armed_flag;

  // Checks the device for a triggered event.
  bool EventAvailable();

  // Reads the data from the device with vme calls.
  void GetEvent(sis_3316 &bundle);

  // Oscillator control commands
  int I2cStart(int osc);
  int I2cStop(int osc);
  int I2cRead(int osc, unsigned char &data, unsigned char ack);
  int I2cWrite(int osc, unsigned char data, unsigned char &ack);

  int SetOscFreqHSN1(int osc, unsigned char hs, unsigned char n1);

  // ADC SPI control commands
  int AdcSpiRead(int gr, int chip, uint addr, uint &msg);
  int AdcSpiWrite(int gr, int chip, uint addr, uint msg);
};

} // ::daq

#endif
