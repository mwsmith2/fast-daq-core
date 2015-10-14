#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_VME_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_VME_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   worker_vme.hh
  
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
#include "worker_base.hh"
#include "common.hh"

namespace daq {

// This class pulls data from a vme device.
// The template is the data structure for the device.
template<typename T>
class WorkerVme : public WorkerBase<T> {

public:
  
  // Ctor params:
  // name - used in naming output data
  // conf - load parameters from a json configuration file
  // num_ch_ - number of channels in the digitizer
  // read_trace_len_ - length of each trace in units of sizeof(uint)
  WorkerVme(std::string name, std::string conf) : 
    WorkerBase<T>(name, conf), 
    num_ch_(SIS_3302_CH), read_trace_len_(SIS_3302_LN) {};

protected:

  const int maxcount_ = 1000;
  int num_ch_;
  uint read_trace_len_;

  int device_;
  uint base_address_; // contained in the conf file.
  
  virtual bool EventAvailable() = 0;
  
  int Read(uint addr, uint &msg);        // A32D32
  int Write(uint addr, uint msg);        // A32D32
  int Read16(uint addr, ushort &msg);    // A16D16
  int Write16(uint addr, ushort msg);    // A16D16
  int ReadTrace(uint addr, uint *trace); // 2eVME (A32)
  int ReadTraceFifo(uint addr, uint *trace); // 2eVMEFIFO (A32)
  int ReadTraceDma32Fifo(uint addr, uint *trace); //BLT32FIFO (A32)
  int ReadTraceMblt64(uint addr, uint *trace); // MBLT64 (A32)
  int ReadTraceMblt64Fifo(uint addr, uint *trace); // MBLT64FIFO (A32)
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
int WorkerVme<T>::Read(uint addr, uint &msg)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static int retval, status, count;

  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  status = (retval = vme_A32D32_read(device_, base_address_ + addr, &msg));
  close(device_);

  if (status != 0) {
    this->LogError("read32  failure at address 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32  vme device 0x%08x, register 0x%08x, data 0x%08x", 
                   base_address_, addr, msg);
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
int WorkerVme<T>::Write(uint addr, uint msg)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  status = (retval = vme_A32D32_write(device_, base_address_ + addr, msg));
  close(device_);

  if (status != 0) {
    this->LogError("write32 failure at address 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("write32 vme device 0x%08x, register 0x%08x, data 0x%08x", 
                   base_address_, addr, msg);
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
int WorkerVme<T>::Read16(uint addr, ushort &msg)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  status = (retval = vme_A32D16_read(device_, base_address_ + addr, &msg));
  close(device_);

  if (status != 0) {
    this->LogError("read16  failure at address 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read16  vme device 0x%08x, register 0x%08x, data 0x%04x", 
                   base_address_, addr, msg);
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
int WorkerVme<T>::Write16(uint addr, ushort msg)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
    static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make our vme call.
  status = (retval = vme_A32D16_write(device_, base_address_ + addr, msg));
  close(device_);

  if (status != 0) {
    this->LogError("write16 failure at address 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("write16 vme device 0x%08x, register 0x%08x, data 0x%04x", 
                   base_address_, addr, msg);
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
int WorkerVme<T>::ReadTrace(uint addr, uint *trace)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static uint num_got;
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  this->LogDump("read_2evme vme device 0x%08x, register 0x%08x, samples %i", 
		 base_address_, addr, read_trace_len_);

  status = (retval = vme_A32_2EVME_read(device_,
                                        base_address_ + addr,
                                        trace,
                                        read_trace_len_,
                                        &num_got));
  close(device_);

  if (status != 0) {
    this->LogError("read32_evme failed at 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32_evme address 0x%08x, ndata asked %i, ndata recv %i", 
                   base_address_ + addr, read_trace_len_, num_got);
  }

  return retval;
}


template<typename T>
int WorkerVme<T>::ReadTraceFifo(uint addr, uint *trace)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static uint num_got;
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  status = (retval = vme_A32_2EVMEFIFO_read(device_,
         				    base_address_ + addr,
         				    trace,
         				    read_trace_len_,
         				    &num_got));
  close(device_);

  if (status != 0) {
    this->LogError("read32_2evmefifo failed at 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32_2evmefifo addr 0x%08x, trace_len %i, ndata recv %i",
                   base_address_ + addr, read_trace_len_, num_got);
  }

  return retval;
}


// Reads a block of data from the specified address offset.  The total
// number of bytes read depends on the read_trace_len_ variable. Uses MBLT64
//
// params:
//   addr - address offset from base_addr_
//   trace - pointer to data being read
//
// return:
//   error code from vme read
template<typename T>
int WorkerVme<T>::ReadTraceMblt64(uint addr, uint *trace)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static uint num_got;
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  status = (retval = vme_A32MBLT64_read(device_,
                                        base_address_ + addr,
                                        trace,
                                        read_trace_len_,
                                        &num_got));
  close(device_);

  if (status != 0) {
    this->LogError("read32_mblt failed at 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32_mblt address 0x%08x, ndata asked %i, ndata recv %i", 
                   base_address_ + addr, read_trace_len_, num_got);
  }

  return retval;
}


// Reads a block of data from the specified address offset.  The total
// number of bytes read depends on the read_trace_len_ variable. Uses MBLT64
//
// params:
//   addr - address offset from base_addr_
//   trace - pointer to data being read
//
// return:
//   error code from vme read
template<typename T>
int WorkerVme<T>::ReadTraceMblt64Fifo(uint addr, uint *trace)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static uint num_got;
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  status = (retval = vme_A32MBLT64FIFO_read(device_,
         				    base_address_ + addr,
         				    trace,
         				    read_trace_len_,
         				    &num_got));
  close(device_);

  if (status != 0) {
    this->LogError("read32_mblt_fifo failed at 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32_mblt_fifo addr 0x%08x, trace_len %i, ndata recv %i", 
                   base_address_ + addr, read_trace_len_, num_got);
  }

  return retval;
}

// Reads a block of data from the specified address offset.  The total
// number of bytes read depends on the read_trace_len_ variable. Uses MBLT64
//
// params:
//   addr - address offset from base_addr_
//   trace - pointer to data being read
//
// return:
//   error code from vme read
template<typename T>
int WorkerVme<T>::ReadTraceDma32Fifo(uint addr, uint *trace)
{
  std::lock_guard<std::mutex> lock(daq::vme_mutex);
  static uint num_got;
  static int retval, status, count;

  // Get the vme device handle.
  count = 0;
  do {
    device_ = open(daq::vme_path.c_str(), O_RDWR);
    usleep(2);
  } while ((device_ < 0) && (count++ < maxcount_));

  // Log an error if we couldn't open it at all.
  if (device_ < 0) {
    this->LogError("failure to find vme device, error %i", device_);
    return device_;
  }

  // Make the vme call.
  status = (retval = vme_A32DMA_D32FIFO_read(device_,
					   base_address_ + addr,
					   trace,
					   read_trace_len_,
					   &num_got));
  close(device_);

  if (status != 0) {
    this->LogError("read32_blt32_fifo failed at 0x%08x", base_address_ + addr);

  } else {

    this->LogDump("read32_blt32_fifo addr 0x%08x, trace_len %i, ndata recv %i", 
                   base_address_ + addr, read_trace_len_, num_got);
  }

  return retval;
}

} // ::daq

#endif
