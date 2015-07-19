#ifndef DAQ_FAST_CORE_INCLUDE_ALTERA_CYCII_HH_
#define DAQ_FAST_CORE_INCLUDE_ALTERA_CYCII_HH_

/*===========================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   altera_cycii.hh

about:  This class interacts with the Altera Cyclone II FPGA via IP-BUS (VME).

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//--- other includes --------------------------------------------------------//
#include "sis3100_vme_dev.hh"
#include "common.hh"

//--- project includes ------------------------------------------------------//

namespace daq {

enum board_id {BOARD_A, BOARD_B, BOARD_C, BOARD_D};

// AlteraCycII contains the basic functionality needed to operate
// the Cyclone 2 FPGA. It inherits vme read/write routines from
// Sis3100VmeDev.
class AlteraCycII : public Sis3100VmeDev {

 public:

  // ctor params:
  //   dev - handle to vme device
  //   base_addr - base address of vme device
  //   block - used to calcuate base address, acromag blocks A-D
  //           get 0xff address space
  //   16 - specifies A16 vme addressing
  //   32 - specifies BLT32 vme block transfers
  AlteraCycII(std::string device, int base_addr, board_id block) :
    Sis3100VmeDev(device, base_addr + 0x100*block, 16, 32) {};

  // Formats and prints the ID data in the ID register of the board.
  void CheckBoardId();

  // Configures the FPGA using the given Intel style hex file.
  int LoadHexCode(std::string filename);
  int LoadHexCode(std::ifstream& in);

 private:

  u_int16_t data_; // forces vme writes to be D16

  //holds a line of hex data, u_int16_t to enforce A16D16
  //A16D8 works theretically, but A16D16 is practically better
  //the CSR address is 0x0 for A16D16, and 0x1 for A16D8
  //the FPGA data register adress is 0x2 for A16D16, and 0x3 for A16D8
  std::vector<u_int16_t> hex_data_;

  const std::string name_ = "AlteraCycII";

  // Read a line of Intel-hex and sets the data vector to the
  // data bytes in the line.
  // returns:
  //   record_type (>0) or error_type (<0)
  int ParseHexRecord(const std::string& record, std::vector<u_int16_t>& data);

};

} // ::daq

#endif
