#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_VME_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_VME_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   daq_worker_vme.hh
  
  about:  Implements the some basic vme functionality to form a base
          class that vme devices can inherit.  It really just defines
	  the most basic read, write, and block transfer vme functions.
	  Sis3100VmeDev is a better class to inherit from if more 
	  functionality is needed.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <iostream>

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"
#include "daq_structs.hh"

namespace daq {

// This class pulls data from a vme device.
// The template is the data structure for the device.
template<typename T>
class DaqWorkerVme : public DaqWorkerBase<T> {

public:
  
  // Ctor params:
  // name - used in naming output data
  // conf - load parameters from a json configuration file
  // num_ch_ - number of channels in the digitizer
  // read_trace_len_ - length of each trace in units of sizeof(uint)
  DaqWorkerVme(std::string name, std::string conf) : 
    DaqWorkerBase<T>(name, conf), 
    num_ch_(SIS_3302_CH), read_trace_len_(SIS_3302_LN) {};

protected:

  int num_ch_;
  int read_trace_len_;

  int device_;
  int base_address_; // contained in the conf file.
  
  virtual bool EventAvailable() = 0;
  
  int Read(int addr, uint &msg);        // A32D32
  int Write(int addr, uint msg);        // A32D32
  int Read16(int addr, ushort &msg);    // A16D16
  int Write16(int addr, ushort msg);    // A16D16
  int ReadTrace(int addr, uint *trace); // MLBT64

};

// Reads 4 bytes from the specified address offset.
//
// params:
//   addr - address offset from base_addr_
//   msg - catches the read data
//
// return:
//   error code from vme read
template<typename T>
int DaqWorkerVme<T>::Read(int addr, uint &msg)
{
  static int retval;
  static int status;

  daq::vme_mutex.lock();
  device_ = open(daq::vme_path.c_str(), O_RDWR);
  status = (retval = vme_A32D32_read(device_, base_address_ + addr, &msg));
  close(device_);
  daq::vme_mutex.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not readable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

// Writes 4 bytes from the specified address offset.
//
// params:
//   addr - address offset from base_addr_
//   msg - data to be written
//
// return:
//   error code from vme write
template<typename T>
int DaqWorkerVme<T>::Write(int addr, uint msg)
{
  static int retval;
  static int status;

  daq::vme_mutex.lock();
  device_ = open(daq::vme_path.c_str(), O_RDWR);
  status = (retval = vme_A32D32_write(device_, base_address_ + addr, msg));
  close(device_);
  daq::vme_mutex.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not writeable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

// Reads 2 bytes from the specified address offset.
//
// params:
//   addr - address offset from base_addr_
//   msg - catches the read data
//
// return:
//   error code from vme read
template<typename T>
int DaqWorkerVme<T>::Read16(int addr, ushort &msg)
{
  static int retval;
  static int status;

  daq::vme_mutex.lock();
  device_ = open(daq::vme_path.c_str(), O_RDWR);
  status = (retval = vme_A32D16_read(device_, base_address_ + addr, &msg));
  close(device_);
  daq::vme_mutex.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not readable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

// Writes 2 bytes from the specified address offset.
//
// params:
//   addr - address offset from base_addr_
//   msg - data to be written
//
// return:
//   error code from vme read
template<typename T>
int DaqWorkerVme<T>::Write16(int addr, ushort msg)
{
  static int retval;
  static int status;

  daq::vme_mutex.lock();
  device_ = open(daq::vme_path.c_str(), O_RDWR);
  status = (retval = vme_A32D16_write(device_, base_address_ + addr, msg));
  close(device_);
  daq::vme_mutex.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not writeable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

// Reads a block of data from the specified address offset.  The total
// number of bytes read depends on the read_trace_len_ variable.
//
// params:
//   addr - address offset from base_addr_
//   trace - pointer to data being read
//
// return:
//   error code from vme read
template<typename T>
int DaqWorkerVme<T>::ReadTrace(int addr, uint *trace)
{
  static int retval;
  static int status;
  static uint num_got;

  daq::vme_mutex.lock();
  device_ = open(daq::vme_path.c_str(), O_RDWR);
  status = (retval = vme_A32_2EVME_read(device_,
                                        base_address_ + addr,
                                        trace,
                                        read_trace_len_,
                                        &num_got));
  close(device_);
  daq::vme_mutex.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Error reading trace at 0x%08x.\n", base_address_ + addr);
    perror(str);
    printf("Asked for: %i, got : %i", read_trace_len_,  num_got);
  }

  return retval;
}

} // ::daq

#endif
