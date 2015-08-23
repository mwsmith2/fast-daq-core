#ifndef DAQ_FAST_CORE_INCLUDE_SIS3100_VME_DEV_HH_
#define DAQ_FAST_CORE_INCLUDE_SIS3100_VME_DEV_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu

  about:  This an base class that wraps many calls from the SIS3100 VME 
  controller driver. The default constructor assumes A32 and MBLT64,
  but this can easily be changed.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"

//--- project includes ------------------------------------------------------//
#include "common.hh"
#include "common_base.hh"

namespace daq {

class Sis3100VmeDev : public daq::CommonBase {

protected:

  // ctor
  Sis3100VmeDev(int addr, int addr_type=32, int mblt_type=64, 
    std::string name="VmeDevice") : addr_(addr), addr_type_(addr_type), 
    mblt_type_(mblt_type), CommonBase(name) {};

  // Open vme device handle from the standard struck location.
  inline void OpenVme() {
    while (dev_ <= 0) {
      dev_ = open(vme_path.c_str(), O_RDWR);
    }
  }

  // Close vme device handle from the standard struck location.
  inline void CloseVme() {
    if (dev_ > 0) {
      close(dev_);
      dev_ = -1;
    }
  }

  // Reset SIS3100 VME controller.
  inline int VmeReset() {
    OpenVme();
    int rc = vmesysreset(dev_);
    CloseVme();
    return rc;
  }

  // The main Read/Writes are effective switch statements calling A16/A24/A32.
  template <typename T>
  inline int Read(const u_int32_t& offset, T &data) {
    if (addr_type_ == 16) {
      return Read16(offset, data);
    } else if (addr_type_ == 24) {
      return Read24(offset, data);
    } else { // default to 32
      return Read32(offset, data);
    }
  }

  template <typename T>
  inline int Write(const u_int32_t& offset, T &data) {
    if (addr_type_ == 16) {
      return Write16(offset, data);
    } else if (addr_type_ == 24) {
      return Write24(offset, data);
    } else { // default to 32
      return Write32(offset, data);
    }
  }

  template <typename T>
  inline int ReadBlock(const u_int32_t& offset, T &data) {
    if (mblt_type_ == 32) {
      if (addr_type_ == 16) {
	return Read16Block32(offset, data);
      } else if (addr_type_ == 24) {
	return Read24Block32(offset, data);
      } else { // default to 32
	return Read32Block(offset, data);
      }
    }
  }
    
  template <typename T>
  inline int WriteBlock(const u_int32_t& offset, T &data) {
    if (mblt_type_ == 32) {
      if (addr_type_ == 16) {
	return Write16Block32(offset, data);
      } else if (addr_type_ == 24) {
	return Write24Block32(offset, data);
      } else { // default to 32
	return Write32Block(offset, data);
      }
    }
  }

  // All the overloaded vme functions.
  // Overloaded A16 Read/Writes.
  inline int Read16(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A16D8_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  } 

  inline int Read16(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A16D16_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Read16(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A16D32_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Write16(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A16D8_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  } 

  inline int Write16(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A16D16_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  inline int Write16(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A16D32_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  // Overloaded A24 Read/Writes.
  inline int Read24(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A32D8_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  } 

  inline int Read24(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A32D16_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Read24(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A32D32_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Read24Block32(const u_int32_t& offset, 
			   u_int32_t *data,
			   const u_int32_t& num_req,
			   u_int32_t& num_got) {

    OpenVme();
    int rc = vme_A24BLT32_read(dev_, addr_ + offset, data, num_req, &num_got);
    CloseVme();
    return rc;
  }

  inline int Read24Block64(const u_int32_t& offset, 
			   u_int32_t *data,
			   const u_int32_t& num_req,
			   u_int32_t& num_got) {

    OpenVme();
    int rc = vme_A24MBLT64_read(dev_, addr_ + offset, data, num_req, &num_got);
    CloseVme();
    return rc;
  }

  inline int Write24(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A32D8_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  } 

  inline int Write24(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A32D16_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  inline int Write24(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A32D32_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  inline int Write24Block32(const u_int32_t& offset, 
			    u_int32_t *data,
			    const u_int32_t& num_req,
			    u_int32_t& num_put) {
    OpenVme();
    int rc = vme_A24BLT32_write(dev_, addr_ + offset, data, num_req, &num_put);
    CloseVme();
    return rc;
  }

  inline int Write24Block64(const u_int32_t& offset, 
			    u_int32_t *data,
			    const u_int32_t& num_req,
			    u_int32_t& num_put) {
    OpenVme();
    int rc = vme_A24MBLT64_write(dev_, addr_ + offset, data, num_req, &num_put);
    CloseVme();
    return rc;
  }

  // Overloaded A32 Read/Writes.
  inline int Read32(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A32D8_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  } 

  inline int Read32(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A32D16_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Read32(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A32D32_read(dev_, addr_ + offset, &data);
    CloseVme();
    return rc;
  }

  inline int Read32Block32(const u_int32_t& offset, 
			   u_int32_t *data,
			   const u_int32_t& num_req,
			   u_int32_t& num_got) {
    OpenVme();
    int rc = vme_A32BLT32_read(dev_, addr_ + offset, data, num_req, &num_got);
    CloseVme();
    return rc;
  }

  inline int Read32Block64(const u_int32_t& offset, 
			   u_int32_t *data,
			   const u_int32_t& num_req,
			   u_int32_t& num_got) {

    OpenVme();
    int rc = vme_A32MBLT64_read(dev_, addr_ + offset, data, num_req, &num_got);
    CloseVme();
    return rc;
  }

  inline int Write32(const u_int32_t& offset, u_int8_t &data) {
    OpenVme();
    int rc = vme_A32D8_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  } 

  inline int Write32(const u_int32_t& offset, u_int16_t &data) {
    OpenVme();
    int rc = vme_A32D16_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  inline int Write32(const u_int32_t& offset, u_int32_t &data) {
    OpenVme();
    int rc = vme_A32D32_write(dev_, addr_ + offset, data);
    CloseVme();
    return rc;
  }

  inline int Write32Block32(const u_int32_t& offset, 
			    u_int32_t *data,
			    const u_int32_t& num_req,
			    u_int32_t& num_put) {
    OpenVme();
    int rc = vme_A32BLT32_write(dev_, addr_ + offset, data, num_req, &num_put);
    CloseVme();
    return rc;
  }

  inline int Write32Block64(const u_int32_t& offset, 
			    u_int32_t *data,
			    const u_int32_t& num_req,
			    u_int32_t& num_put) {
    OpenVme();
    int rc = vme_A32MBLT64_write(dev_, addr_ + offset, data, num_req, &num_put);
    CloseVme();
    return rc;
  }

 private:

  int dev_;
  int addr_;
  int addr_type_;
  int mblt_type_;

};

} // ::daq

#endif
