#include "dio_mux_controller.hh"

namespace daq {

DioMuxController::DioMuxController(std::string dev, int board_addr, board_id bid) :
  io_board_(dev, board_addr, bid)
{
  // Instantiate the carrier board class.
  io_board_.CheckBoardId();

  // Configure the channel map, since it's kind of wonky.
  int idx = 1;
  for (int i = 31; i >= 25; --i) {
    channel_map_[idx++] = i;
  }

  for (int i = 23; i >= 18; --i) {
    channel_map_[idx++] = i;
  }

  for (int i = 15; i >= 9; --i) {
    channel_map_[idx++] = i;
  }
}  

void DioMuxController::AddMux(std::string mux_name, int port_idx)
{
  mux_port_map_[mux_name] = port_idx;
}

void DioMuxController::SetMux(std::string mux_name, int mux_ch)
{
  io_board_.WritePort(mux_port_map_[mux_name], channel_map_[mux_ch]);

  // Need to wait due to unit's capacitance.
  usleep(10);
}

} // ::daq
