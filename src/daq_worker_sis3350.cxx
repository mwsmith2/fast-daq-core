#include "daq_worker_sis3350.hh"

namespace daq {

DaqWorkerSis3350::DaqWorkerSis3350(string name, string conf) : DaqWorkerVme<sis_3350>(name, conf)
{
  num_ch_ = SIS_3350_CH;
  read_trace_len_ = SIS_3350_LN / 2 + 4;

  LoadConfig();
}

void DaqWorkerSis3350::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  // Get the base address.  Needs to be converted from hex.
  base_address_ = std::stoi(conf.get<string>("base_address"), nullptr, 0);

  int ret;
  uint msg = 0;

  // Check for device.
  Read(0x0, msg);
  printf("sis3350 found at 0x%08x\n.", base_address_);

  // Reset device.
  msg = 1;
  Write(0x400, msg);

  // Get and print device ID.
  msg = 0;
  Read(0x4, msg);
  
  printf("sis3350 ID: %04x, maj rev: %02x, min rev: %02x\n",
	       msg >> 16, (msg >> 8) & 0xff, msg & 0xff);

  // Set and check the control/status register.
  msg = 0;

  if (conf.get<bool>("invert_ext_lemo")) {
    msg |= 0x10; // invert EXT TRIG
  }

  if (conf.get<bool>("user_led_on")) {
    msg |= 0x1; // LED on
  }

  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= 0x00110011; // zero reserved bits
  Write(0x0, msg);

  msg = 0;
  Read(0x0, msg);

  printf("EXT LEMO set to %s\n", ((msg & 0x10) == 0x10) ? "NIM" : "TTL");
  printf("user LED turned %s\n", (msg & 0x1) ? "ON" : "OFF");

  // Set to the acquisition register.
  msg = 0;
  if (conf.get<bool>("ringbuffer_mode")) {

    msg |= 0x1; //sync ring buffer mode

  } else if (conf.get<bool>("direct_memory_mode")) {

    msg |= 0x5; // direct memory trigger start mode

  } else {

    msg |= 0x1; // default to sync ring buffer mode
  }
    
  //msg |= 0x1 << 5; //enable multi mode
  //msg |= 0x1 << 6; //enable internal (channel) triggers

  if (conf.get<bool>("enable_ext_lemo")) {
    msg |= 0x1 << 8; //enable EXT LEMO
  }

  //msg |= 0x1 << 9; //enable EXT LVDS
  //msg |= 0x0 << 12; // clock source: synthesizer

  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= ~0xcc98cc98; //reserved bits

  printf("sis 3350 setting acq reg to 0x%08x\n", msg);
  Write(0x10, msg);

  msg = 0;
  Read(0x10, msg);
  printf("sis 3350 acq reg reads 0x%08x\n", msg);

  // direct memory sample length register
  msg = SIS_3350_LN;
  Write(0x18, msg);


  // Set the synthesizer register.
  msg = 0x14; //500 MHz
  Write(0x1c, msg);

  // Set the memory page.
  msg = 0; //first 8MB chunk
  Write(0x34, msg);

  // Set the trigger output.
  msg = 0;
  msg |= 0x1; // LEMO IN -> LEMO OUT
  Write(0x38, msg);

  // Set ext trigger threshold.
  //first load data, then clock in, then ramp
  //msg = 35000; // +1.45V TTL
  msg = 31500; // NIM
  Write(0x54, msg);

  msg = 0;
  msg |= 0x1 << 4; //TRG threshold
  msg |= 0x1; //load shift register DAC
  Write(0x50, msg);

  uint timeout_max = 1000;
  uint timeout_cnt = 0;
  do {

    msg = 0;
    Read(0x50, msg);
    timeout_cnt++;

  } while (((msg & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

  if (timeout_cnt == timeout_max) {
    printf("error loading ext trg shift reg\n");
  }

  msg = 0;
  msg |= 0x1 << 4; //TRG threshold
  msg |= 0x2; //load DAC
  Write(0x50, msg);

  timeout_cnt = 0;
  do {

    msg = 0;
    Read(0x50, msg);
    timeout_cnt++;

  } while (((msg & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

  if (timeout_cnt == timeout_max) {
    printf("error loading ext trg dac\n");
  }

  //board temperature
  msg = 0;

  Read(0x70, msg);
  printf("sis3350 board temperature: %.2f degC\n", (float)msg / 4.0);

  //ring buffer sample length
  msg = SIS_3350_LN;
  Write(0x01000020, msg);

  //ring buffer pre-trigger sample length
  msg = std::stoi(conf.get<string>("pretrigger_samples"), nullptr, 0);
  Write(0x01000024, msg);

  //range -1.5 to +0.3 V
  uint ch = 0;
  //DAC offsets
  for (auto &val : conf.get_child("channel_offset")) {

    int offset = 0x02000050;
    offset |= (ch >> 1) << 24;

    float volts = val.second.get_value<float>();
    msg = (int)(33000 + 3377.77 * volts + 0.5);
    Write(offset | 4, msg);

    msg = 0;
    msg |= (ch % 2) << 4;
    msg |= 0x1; //load shift register DAC
    Write(offset, msg);

    timeout_max = 1000;
    timeout_cnt = 0;

    do {
      msg = 0;
      Read(offset, msg);
      timeout_cnt++;
    } while (((msg & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

    if (timeout_cnt == timeout_max) {
      printf("error loading ext trg shift reg\n");
    }

    msg = 0;
    msg |= (ch % 2) << 4;
    msg |= 0x2; //load DAC
    Write(offset, msg);

    timeout_cnt = 0;
    do {
      msg = 0;
      Read(offset, msg);
      timeout_cnt++;
    } while (((msg & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

    if (timeout_cnt == timeout_max) {
      printf("error loading adc dac\n");
    }

    ++ch;
  }

  usleep(20000); // Magic sleep needed before setting gain.

  //gain - factory default 18 -> 5V
  ch = 0;
  for (auto &val : conf.get_child("channel_gain")) {
    //  for (ch = 0; ch < SIS_3350_CH; ch++) {
    msg = val.second.get_value<int>();

    int offset = 0x02000048;
    offset |= (ch >> 1) << 24;
    offset |= (ch % 2) << 2;
    Write(offset, msg);
    printf("adc %d gain %d\n", ch, msg);

    ++ch;
    usleep(20000);
  }

  //arm the logic
  uint armit = 1;
  Write(0x410, armit);
} // LoadConfig

void DaqWorkerSis3350::WorkLoop()
{
  t0_ = high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static sis_3350 bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();

      } else {
	
	std::this_thread::yield();
	usleep(daq::kShortSleep);

      }
    }

    std::this_thread::yield();
    usleep(daq::kLongSleep);
  }
}

sis_3350 DaqWorkerSis3350::PopEvent()
{
  static sis_3350 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {

    sis_3350 str;
    queue_mutex_.unlock();
    return str;

  } else if (!data_queue_.empty()) {

    // Copy the data.
    data = data_queue_.front();
    data_queue_.pop();
    
    // Check if this is that last event.
    if (data_queue_.size() == 0) has_event_ = false;
    
    queue_mutex_.unlock();
    return data;
  }
}


bool DaqWorkerSis3350::EventAvailable()
{
  // Check acq reg.
  static uint msg = 0;
  static bool is_event;

  Read(0x10, msg);
  is_event = !(msg & 0x10000);
  
  if (is_event) {
    // Rearm the logic.
    uint armit = 1;
    Write(0x410, armit);
  }

  return is_event;
}

// Pull the event.
void DaqWorkerSis3350::GetEvent(sis_3350 &bundle)
{
  int ch, offset, ret = 0;
  bool is_event = true;

  // Check how long the event is.
  //expected SIS_3350_LN + 8
  
  uint next_sample_address[4] = {0, 0, 0, 0};
  
  for (ch = 0; ch < SIS_3350_CH; ch++) {
    
    offset = 0x02000010;
    offset |= (ch >> 1) << 24;
    offset |= (ch & 0x1) << 2;

    Read(offset, next_sample_address[ch]);
  }

  // Get the system time.
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();

  //todo: check it has the expected length
  uint trace[4][SIS_3350_LN / 2 + 4];

  for (ch = 0; ch < SIS_3350_CH; ch++) {
    offset = (0x4 + ch) << 24;
    ReadTrace(offset, trace[ch]);
  }

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3350_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = trace[ch][1] & 0xfff;
    bundle.device_clock[ch] |= (trace[ch][1] & 0xfff0000) >> 4;
    bundle.device_clock[ch] |= (trace[ch][0] & 0xfffULL) << 24;
    bundle.device_clock[ch] |= (trace[ch][0] & 0xfff0000ULL) << 20;

    uint idx;
    for (idx = 0; idx < SIS_3350_LN / 2; idx++) {
      bundle.trace[ch][2 * idx] = trace[ch][idx + 4] & 0xfff;
      bundle.trace[ch][2 * idx + 1] = (trace[ch][idx + 4] >> 16) & 0xfff;
    }
  }
}

} // ::daq
