#ifndef DAQ_FAST_CORE_INCLUDE_MUX_CONTROL_BOARD_HH_
#define DAQ_FAST_CORE_INCLUDE_MUX_CONTROL_BOARD_HH_

/*============================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   mux_control_board.hh

about: Allows easy intuitive control of the multiplexers connected to 
       acromag ip470 digital I/O boards.

\*============================================================================*/

//--- std includes -----------------------------------------------------------//
#include <map>
#include <string>

//--- other includes ---------------------------------------------------------//

//--- project includes -------------------------------------------------------//
#include "acromag_ip470a.hh"

namespace daq {

class MuxControlBoard
{
 public:
  
  // Ctor params:
  MuxControlBoard(std::string dev, int board_addr, board_id bid);

  void AddMux(std::string mux_name, int port_idx);
  void SetMux(std::string mux_name, int mux_ch);
  bool HasMux(std::string mux_name) { 
    return (mux_port_map_.count(mux_name) > 0); 
  };

 private:

  AcromagIp470a io_board_;
  std::map<std::string, int> mux_port_map_;
  std::map<int, int> channel_map_;
};

} // ::daq

#endif
