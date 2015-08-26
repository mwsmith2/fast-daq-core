#include "dio_stepper_motor.hh"

namespace daq {

DioStepperMotor::DioStepperMotor(int board_addr, board_id bid, 
                                 std::string conf_file) :
  io_board_(board_addr, bid), conf_file_(conf_file)
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  input_port_ = conf.get<int>("input_port.port_num");
  aft_bumper_bit_ = conf.get<int>("input_port.aft_bumper_bit");
  bow_bumper_bit_ = conf.get<int>("input_port.bow_bumper_bit");
  
  output_port_ = conf.get<int>("output_port.port_num");
  direction_bit_ = conf.get<int>("output_port.direction_bit");
  enable_bit_ = conf.get<int>("output_port.enable_bit");
  clock_bit_ = conf.get<int>("output_port.clock_bit");

  steps_per_inch_ = 1.0 / conf.get<double>("inches_per_step");
  steps_per_cm_ = steps_per_inch_ / 2.5400;

  if (conf.get<bool>("use_sextets", false)) {
    io_board_.EnableSextets();

  } else {

    io_board_.EnableOctets();
  }

  // Mask the input port.
  u_int8_t data = 0x1 << input_port_;
  int rc = io_board_.WritePort(7, data);
  rc = io_board_.WritePort(input_port_, 0x0);

  thread_live_ = true;
  move_it_ = false;

  clock_tick_ = int(1.0e6 / conf.get<float>("clock_rate") + 0.5);
  clock_thread_ = std::thread(&DioStepperMotor::ClockLoop, this);
}

int DioStepperMotor::ForwardDirection()
{
  u_int8_t data;
  board_guard_.lock();

  // Read to preserver old info.
  io_board_.ReadPort(output_port_, data);
  
  // Set direction bit high.
  data |= 0x1 << direction_bit_;
  
  // Write the data back.
  io_board_.WritePort(output_port_, data);
  board_guard_.unlock();
  
  return 0;
}

int DioStepperMotor::BackwardDirection()
{
  u_int8_t data;
  board_guard_.lock();

  // Read to preserver old info.
  io_board_.ReadPort(output_port_, data);
  
  // Set direction bit low.
  data &= 0xff^(0x1 << direction_bit_); 

  // Write the data back.
  io_board_.WritePort(output_port_, data);
  board_guard_.unlock();
  
  return 0;
}

void DioStepperMotor::ClockLoop()
{
  while(thread_live_) {
    
    while(move_it_ && thread_live_) {
      
      static u_int8_t data;
      
      board_guard_.lock();
      
      io_board_.ReadPort(input_port_, data);
 
      // Make sure we aren't at the end of the track.
      if (0x1 & (data >> aft_bumper_bit_) ||
	  0x1 & (data >> bow_bumper_bit_) ){
	move_it_ = false;
      }
      
      // Read to preserve old info.
      io_board_.ReadPort(output_port_, data);
      
      // Flip clock bit high.
      data |= 0x1 << clock_bit_;
      
      // Write the data back.
      io_board_.WritePort(output_port_, data);
      board_guard_.unlock();

      // Wait half a cycle.
      usleep(clock_tick_ / 2);
      
      // Flip the clock bit again.
      board_guard_.lock();
      io_board_.ReadPort(output_port_, data);
      data &= ~(0x1 << clock_bit_);
      io_board_.WritePort(output_port_, data);
      board_guard_.unlock();

      
      usleep(clock_tick_ / 2);
      std::this_thread::yield();
    }
  }
}

int DioStepperMotor::StepForward(double num_steps)
{
  ForwardDirection();
  move_it_ = true;
  usleep(clock_tick_ * num_steps * 4);
  move_it_ = false;

  return 0;
}

int DioStepperMotor::StepBackward(double num_steps)
{
  BackwardDirection();
  move_it_ = true;
  usleep(clock_tick_ * num_steps * 4);
  move_it_ = false;

  return 0;
}

int DioStepperMotor::MoveInchesForward(double in)
{
  StepForward(in * steps_per_inch_);

  return 0;
}

int DioStepperMotor::MoveInchesBackward(double in)
{
  StepBackward(in * steps_per_inch_);

  return 0;
}

int DioStepperMotor::MoveCmForward(double cm)
{
  StepForward(cm * steps_per_cm_);

  return 0;
}

int DioStepperMotor::MoveCmBackward(double cm)
{
  StepBackward(cm * steps_per_cm_);

  return 0;
}


} // ::daq
