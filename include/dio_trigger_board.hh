#ifndef DAQ_FAST_CORE_INCLUDE_DIO_TRIGGER_BOARD_HH_
#define DAQ_FAST_CORE_INCLUDE_DIO_TRIGGER_BOARD_HH_

/*============================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   dio_trigger_board.hh

about: Allows easy intuitive logic trigger based on acromag ip470 
       digital I/O boards.

\*============================================================================*/

//--- std includes -----------------------------------------------------------//
#include <unistd.h>
#include <string>

//--- other includes ---------------------------------------------------------//

//--- project includes -------------------------------------------------------//
#include "acromag_ip470a.hh"

namespace daq {

class DioTriggerBoard
{
 public:
  
  // Ctor params:
  DioTriggerBoard(int board_addr, board_id bid, int trg_port);

  // Set the proper acromag port used for sending TTL triggers.
  void SetTriggerPort(int trg_port) { trg_port_ = trg_port; };

  // Fire TTL pulse
  void FireTrigger(int trg_bit=0, int length_us=10);

 private:

  AcromagIp470a io_board_;
  int trg_port_;
};

} // ::daq

#endif
