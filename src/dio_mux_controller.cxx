#include "dio_mux_controller.hh"

namespace daq {

DioMuxController::DioMuxController(int board_addr,
                                   board_id bid,
                                   bool enable_sextets) :
  io_board_(board_addr, bid, enable_sextets)
{
  // Instantiate the carrier board class.
  io_board_.CheckBoardId();

  // Configure the channel map, since it's kind of wonky.
  int idx = 1;
  for (int i = 31; i >= 25; --i) {
    old_channel_map_[idx++] = i;
  }

  for (int i = 23; i >= 18; --i) {
    old_channel_map_[idx++] = i;
  }

  for (int i = 15; i >= 9; --i) {
    old_channel_map_[idx++] = i;
  }

  for (int i = 1; i < 25; ++i) {
    new_channel_map_[i] = 32- i;
  }
}

void DioMuxController::AddMux(std::string mux_name, int port_idx, bool old_mux)
{
  mux_port_map_[mux_name] = port_idx;

  if (old_mux) {
    mux_channel_map_[mux_name] = old_channel_map_;
  } else {
    mux_channel_map_[mux_name] = new_channel_map_;
  }
}

void DioMuxController::SetMux(std::string mux_name, int mux_ch)
{
  io_board_.WritePort(mux_port_map_[mux_name], mux_channel_map_[mux_name][mux_ch]);

  // Need to wait due to unit's capacitance.
  usleep(10);
}

} // ::daq
