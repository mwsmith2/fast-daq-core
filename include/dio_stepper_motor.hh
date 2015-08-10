#ifndef DAQ_FAST_CORE_INCLUDE_DIO_STEPPER_MOTOR_HH_
#define DAQ_FAST_CORE_INCLUDE_DIO_STEPPER_MOTOR_HH_

/*============================================================================*\

author: Matthias W. Smith
email:  mwsmith2@uw.edu
file:   dio_stepper_motor.hh

about: A class interface to control the stepper motor at CENPA with an
       acromag ip470a digital I/O board.

\*============================================================================*/

//--- std includes -----------------------------------------------------------//
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

//--- other includes ---------------------------------------------------------//
#include <unistd.h>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

//--- project includes -------------------------------------------------------//
#include "acromag_ip470a.hh"

namespace daq {

class DioStepperMotor
{
 public:
  
  // Ctor params:
  DioStepperMotor(std::string devpath, int board_addr, 
                  board_id bid, std::string conf_file);

  // Dtor
  ~DioStepperMotor() {
    thread_live_ = false;

    if (clock_thread_.joinable()) {
      clock_thread_.join();
    }
  };

  // Set the proper acromag port used for stepper motor input pins.
  void SetInputPort(int input_port) { input_port_ = input_port; };

  // Set the proper acromag port used for stepper motor input pins.
  void SetOutputPort(int output_port) { output_port_ = output_port; };

  // Movement functions, scales were determined empirically.
  int MoveInchesForward(double in=1.0);
  int MoveInchesBackward(double in=1.0);
  int MoveCmForward(double cm=1.0);
  int MoveCmBackward(double cm=1.0);
  int StepForward(double num_steps);
  int StepBackward(double num_steps);

  // Stop/start functions.
  int EnableMotor() { move_it_ = true; };
  int DisableMotor() { move_it_ = false; };

  // Accessors for the distance conversions.
  double steps_per_inch() { return steps_per_inch_; };
  double steps_per_cm() { return steps_per_cm_; };

 private:

  int input_port_;
  int aft_bumper_bit_;
  int bow_bumper_bit_;

  int output_port_;
  int direction_bit_;
  int enable_bit_;
  int clock_bit_;
  
  int clock_tick_; // in microseconds
  double steps_per_inch_;
  double steps_per_cm_;
  std::atomic<bool> thread_live_;
  std::atomic<bool> move_it_;
  std::string conf_file_;

  AcromagIp470a io_board_;

  std::mutex board_guard_;
  std::thread clock_thread_;
  
  // Set the direction on the motor, could be relative if set up changes.
  int ForwardDirection();
  int BackwardDirection();

  // Get its own thread to act as the clock driving the motor.
  void ClockLoop();
};

} // ::daq

#endif
