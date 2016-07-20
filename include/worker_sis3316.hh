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
  //     "base_address": "0x20000000",
  //     "enable_ext_trg": true,
  //     "enable_int_trg": false,
  //     "invert_ext_trg": false,
  //     "enable_ext_clk": false,
  //     "oscillator_hs": 5,
  //     "oscillator_n1": 8,
  //     "iob_tap_delay": "0x1020",
  //     "set_voltage_offset": true,
  //     "dac_voltage_offset": "0x8000",
  //     "pretrigger_samples": "0x0"
  // }
  void LoadConfig();

  // The threaded loop that polls for data and pushes events on the queue.
  void WorkLoop();

  // Returns the oldest event on the data queue.
  sis_3316 PopEvent();

 private:

  // Register constants which are substrings of those given by Struck.
  const static uint CONTROL_STATUS = 0x0;
  const static uint MODID = 0x4;
  const static uint HARDWARE_VERSION = 0x1C;
  const static uint INTERNAL_TEMPERATURE = 0x20;
  const static uint ADC_CLK_OSC_I2C = 0x40;
  const static uint SAMPLE_CLOCK_DISTRIBUTION_CONTROL = 0x50;
  const static uint NIM_CLK_MULTIPLIER_SPI = 0x54;
  const static uint NIM_INPUT_CONTROL = 0x5c;
  const static uint ACQUISITION_CONTROL = 0x60;
  const static uint LEMO_OUT_CO_SELECT = 0x70;
  const static uint DATA_TRANSFER_ADC1_4_CTRL = 0x80;
  const static uint DATA_TRANSFER_ADC1_4_STATUS = 0x90;

  // Key registers.
  const static uint KEY_RESET = 0x400;
  const static uint KEY_DISARM = 0x414;
  const static uint KEY_TRIGGER = 0x418;
  const static uint KEY_TIMESTAMP_CLR = 0x41c;
  const static uint KEY_DISARM_AND_ARM_BANK1 = 0x420;
  const static uint KEY_DISARM_AND_ARM_BANK2 = 0x424;
  const static uint KEY_ADC_CLOCK_DCM_RESET = 0x438;

  // These registers are evenly spaced by 0x1000 between all 4 ADCs.
  const static uint CH1_4_INPUT_TAP_DELAY = 0x1000;
  const static uint CH1_4_ANALOG_CTRL = 0x1004;
  const static uint CH1_4_DAC_OFFSET_CTRL = 0x1008;
  const static uint CH1_4_SPI_CTRL = 0x100c;
  const static uint CH1_4_EVENT_CONFIG = 0x1010;
  const static uint CH1_4_CHANNEL_HEADER = 0x1014;
  const static uint CH1_4_ADDRESS_THRESHOLD = 0x1018;
  const static uint CH1_4_TRIGGER_GATE_WINDOW_LENGTH = 0x101c;
  const static uint CH1_4_RAW_DATA_BUFFER_CONFIG = 0x1020;
  const static uint CH1_4_PRE_TRIGGER_DELAY = 0x1028;
  const static uint CH1_4_DATAFORMAT_CONFIG = 0x1030;
  const static uint CH1_4_EXTENDED_RAW_DATA_BUFFER_CONFIG = 0x1098;
  const static uint CH1_4_FIRMWARE = 0x1100;
  const static uint CH1_4_STATUS = 0x1104;
  const static uint CH1_4_DAC_OFFSET_READBACK = 0x1108;
  const static uint CH1_4_SPI_READBACK = 0x110c;
  const static uint CH1_ACTUAL_SAMPLE_ADDRESS = 0x1110;
  const static uint CH1_PREVIOUS_SAMPLE_ADDRESS = 0x1120;

  // Other constants
  const static uint kAdcRegOffset = 0x1000;
  const static uint kMaxPoll = 1000;

  // Variables
  std::chrono::high_resolution_clock::time_point t0_;
  std::atomic<bool> bank2_armed_flag;

  // Checks the device for a triggered event.
  bool EventAvailable();

  // Reads the data from the device with vme calls.
  void GetEvent(sis_3316 &bundle);

  // Auxilliary control utilities defined below:
  // internal oscillator via I2C, see SI570 manual
  // individual ADCs via SPI, see AD9643 manual
  // NIM clock multiplier, see SI5325 manual

  // Internal oscillator control commands
  int I2cStart(int osc);
  int I2cStop(int osc);
  int I2cRead(int osc, unsigned char &data, unsigned char ack);
  int I2cWrite(int osc, unsigned char data, unsigned char &ack);

  // Set two frequency divider variables to control speed.
  int SetOscFreqHSN1(int osc, unsigned char hs, unsigned char n1);

  // ADC SPI control commands
  int AdcSpiRead(int gr, int chip, uint addr, uint &msg);
  int AdcSpiWrite(int gr, int chip, uint addr, uint msg);

  // External clock multiplier SPI.
  int Si5325Read(uint addr, uint &msg);
  int Si5325Write(uint addr, uint msg);
};

} // ::daq

#endif
