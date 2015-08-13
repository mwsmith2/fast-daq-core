#ifndef DAQ_FAST_CORE_INCLUDE_DIO_MUX_CONTROLLER_HH_
#define DAQ_FAST_CORE_INCLUDE_DIO_MUX_CONTROLLER_HH_

/*============================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   dio_mux_controller.hh

about: Allows intuitive control of the multiplexers connected to 
       acromag ip470 digital I/O boards.

\*============================================================================*/

//--- std includes -----------------------------------------------------------//
#include <map>
#include <string>

//--- other includes ---------------------------------------------------------//

//--- project includes -------------------------------------------------------//
#include "acromag_ip470a.hh"

namespace daq {

class DioMuxController
{
 public:
  
  // Ctor params:
  DioMuxController(std::string dev, int board_addr, board_id bid);

  // Add a new multiplexer controlled by the acromag dio.
  void AddMux(std::string mux_name, int port_idx);

  // Set for the next command.
  void SetMux(std::string mux_name, int mux_ch);

  // Check if a multiplexer is already present.
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
