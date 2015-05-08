#include "worker_sis3350.hh"

namespace daq {

WorkerSis3350::WorkerSis3350(std::string name, std::string conf) : 
  WorkerVme<sis_3350>(name, conf)
{
  num_ch_ = SIS_3350_CH;
  read_trace_len_ = SIS_3350_LN / 2 + 4;

  LoadConfig();
}

void WorkerSis3350::LoadConfig()
{ 
  using std::cout;
  using std::endl;
  using std::cerr;
  using std::string;

  int rc = 0;
  uint msg = 0;
  char str[256];

  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  // Get the device filestream.  If it isn't open, open it.
  string dev_path = conf.get<string>("device");

  if (logging_on) {
    logstream << name_ << " (WorkerSis3350, *" << this << "): ";
    logstream << "is initializing." << device_ << endl;
  }

  // Get the base address.  Needs to be converted from hex.
  base_address_ = std::stoi(conf.get<string>("base_address"), nullptr, 0);

  // Check for device.
  rc = Read(0x0, msg);
  if (rc >= 0) {

    sprintf(str, " found at vme address: 0x%08x", base_address_);
    WriteLog(name_ + std::string(str));

  } else {
    WriteLog("Failed to communicate with VME device.");
  }

  // Reset device.
  msg = 1;
  Write(0x400, msg);

  // Get and print device ID.
  msg = 0;
  Read(0x4, msg);
  
  if (logging_on) {
    sprintf(str, " ID: %04x, maj rev: %02x, min rev: %02x",
	    msg >> 16, (msg >> 8) & 0xff, msg & 0xff);
    logstream << name_ << str << endl;
  }

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

  if (logging_on) {
    sprintf(str, " EXT LEMO: %s", ((msg & 0x10) == 0x10) ? "NIM" : "TTL");
    logstream << name_ << str << endl;

    sprintf(str, " user LED: %s", (msg & 0x1) ? "ON" : "OFF");
    logstream << name_ << str << endl;
  }

  // Set to the acquisition register.
  msg = 0;
  msg |= 0x1; //sync ring buffer mode
  //msg |= 0x1 << 5; //enable multi mode
  //msg |= 0x1 << 6; //enable internal (channel) triggers

  if (conf.get<bool>("enable_ext_lemo")) {
    msg |= 0x1 << 8; //enable EXT LEMO
  }

  //msg |= 0x1 << 9; //enable EXT LVDS
  //msg |= 0x0 << 12; // clock source: synthesizer

  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= ~0xcc98cc98; //reserved bits

  Write(0x10, msg);

  msg = 0;
  Read(0x10, msg);
  if (logging_on) {
    sprintf(str, " ACQ register set to: 0x%08x", msg);
    logstream << name_ << str << endl;
  }

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
  msg = 35000; // +1.45V TTL
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
    cerr << name_ << ": error loading ext trg shift reg" << endl;
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
    cerr << name_ << ": error loading ext trg dac" << endl;
  }

  //board temperature
  msg = 0;

  Read(0x70, msg);
  if (logging_on) {
    sprintf(str, " board temperature: %.2f degC", (float)msg / 4.0);
    logstream << name_ << str << endl;
  }
  
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
      cerr << name_ << ": error loading ext trg shift reg" << endl;
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
    
    if (logging_on) {
      sprintf(str, " adc %d gain %d\n", ch, msg);
      logstream << name_ << str << endl;
    }

    ++ch;
    usleep(20000);
  }

  uint armit = 1;
  rc = Write(0x410, armit);
} // LoadConfig

void WorkerSis3350::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    // Grab the event if we have one.
    if (EventAvailable()) {
      
      static sis_3350 bundle;
      GetEvent(bundle);
      
      queue_mutex_.lock();
      data_queue_.push(bundle);
      has_event_ = true;

      // Drop old events
      if (data_queue_.size() > max_queue_size_)
	data_queue_.pop();

      queue_mutex_.unlock();
      
    } else {
      
      std::this_thread::yield();
      usleep(daq::short_sleep);
    }


    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

sis_3350 WorkerSis3350::PopEvent()
{
  static sis_3350 data;

  queue_mutex_.lock();

  if (data_queue_.empty()) {
    sis_3350 str;
    
    queue_mutex_.unlock();
    return str;
  }

  // Copy the data.
  data = data_queue_.front();
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  queue_mutex_.unlock();
  return data;
}


bool WorkerSis3350::EventAvailable()
{
  // Check acq reg.
  static uint msg = 0;
  static bool is_event;
  static int count, rc;

  count = 0;
  rc = 0;
  do {
    rc = Read(0x10, msg);
    ++count;
  } while ((rc < 0) && (count < 100));
 
  is_event = !(msg & 0x10000);

  if (is_event) {
    // rearm the logic
    uint armit = 1;

    count = 0;
    rc = 0;
    do {
      rc = Write(0x410, armit);
      ++count;
    } while ((rc < 0) && (count < 100));

    return is_event;

  } else {

    return false;
  }
}
  // // Check acq reg.
  // static uint msg = 0;
  // static bool is_event;

  // Read(0x10, msg);
  // is_event = !(msg & 0x10000);
  
  // // Rearm the logic if we are runnin.
  // if (is_event && go_time_) {
  //   uint armit = 1;
  //   Write(0x410, armit);
  // }

  // return is_event;
  //}

// Pull the event.
void WorkerSis3350::GetEvent(sis_3350 &bundle)
{
  using namespace std::chrono;

  int ch, offset, rc = 0;
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
