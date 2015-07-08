#include "worker_sis3302.hh"

namespace daq {

WorkerSis3302::WorkerSis3302(std::string name, std::string conf) : 
  WorkerVme<sis_3302>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3302_CH;
  read_trace_len_ = SIS_3302_LN / 2; // only for vme ReadTrace
}

void WorkerSis3302::LoadConfig()
{ 
  using std::string;

  int rc;
  uint msg = 0;
  char str[256];

  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);
  
  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoul(conf.get<string>("base_address"), nullptr, 0);

  // Read the base register.
  rc = Read(0x0, msg);
  if (rc != 0) {

    LogError("could not find SIS3302 device at 0x%08x", base_address_);

  } else {
   
    LogMessage("SIS3302 Found at 0x%08x", base_address_);
  }

  // Reset the device.
  rc = Write(0x400, 0x1);
  if (rc != 0) {
    LogError("failure writing sis3302 reset register");
  }

  // Get device ID.
  rc = Read(0x4, msg);
  if (rc != 0) {

    LogError("failed reading device ID");

  } else {

    LogMessage("ID: %04x, maj rev: %02x, min rev: %02x",
               msg >> 16, (msg >> 8) & 0xff, msg & 0xff);
  }

  // Read control/status register.
  msg = 0;
  if (conf.get<bool>("invert_ext_lemo")) {
    msg |= 0x10; // invert EXT TRIG
  }
  
  if (conf.get<bool>("user_led_on")) {
    msg |= 0x1; // LED on
  }

  // Toggle reserved bits
  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= ~0xfffefffe;

  rc = Write(0x0, msg);
  if (rc != 0) {
    LogError("failed setting the control register");
  }
  
  rc = Read(0x0, msg);
  if (rc != 0) {

    LogError("failed reading status/control register");

  } else {

    LogMessage("User LED: %s", (msg & 0x1) ? "ON" : "OFF");
  }

  // Set Acquisition register.
  msg = 0;
  if (conf.get<bool>("enable_int_stop", true))
    msg |= 0x1 << 6; //enable internal stop trigger

  if (conf.get<bool>("enable_ext_lemo", true))
    msg |= 0x1 << 8; //enable EXT LEMO

  // Set the clock source: 0x0 = Internal, 100MHz
  msg |= conf.get<int>("clock_settings", 0x0) << 12;
  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= 0x7df07df0; // zero reserved bits / disable bits

  rc = Write(0x10, msg);
  if (rc != 0) {
    LogError("failed writing acquisition register");
  }

  rc = Read(0x10, msg);
  if (rc != 0) {

    LogError("failed reading acquisition register");

  } else {

    LogDebug("acquisition register set to: 0x%08x", msg);
  }

  // Set the start delay.
  msg = conf.get<int>("start_delay", 0);

  rc = Write(0x14, msg);
  if (rc != 0) {
    LogError("failed to set start delay");
  }

  // Set the stop delay.
  msg = conf.get<int>("stop_delay", 0);
  rc = Write(0x18, msg);
  if (rc != 0) {
    LogError("failed to set stop delay");
  }

  // Read event configuration register.
  rc = Read(0x02000000, msg);
  if (rc != 0) {
    LogError("failed to read event configuration register");
  }

  // Set event configure register with changes
  if (conf.get<bool>("enable_event_length_stop", true)) {
    msg = 0x1 << 5; 
  }

  rc = Write(0x01000000, msg);
  if (rc != 0) {
    LogError("failed to set event configuration register");
  }
  
  // Event length register - odd setting method (see manual).
  msg = (SIS_3302_LN - 4 + 512) & 0xfffffc; // @hack - sets number of samples
  rc = Write(0x01000004, msg);
  if (rc != 0) {
    LogError("failed to set event length");
  }

  // Set the pre-trigger buffer length.
  msg = std::stoi(conf.get<string>("pretrigger_samples", "0x0"), nullptr, 0);
  rc = Write(0x01000060, msg);
  if (rc != 0) {
    LogError("failed to set pre-trigger buffer");
  }

  // Memory page
  msg = 0; //first 8MB chunk
  rc = Write(0x34, msg);
  if (rc != 0) {
    LogError("failed to set memory page");
  }
} // LoadConfig

void WorkerSis3302::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static sis_3302 bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();

      } else {

	std::this_thread::yield();
	usleep(daq::short_sleep);
      }
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

sis_3302 WorkerSis3302::PopEvent()
{
  static sis_3302 data;

  if (data_queue_.empty()) {
    sis_3302 str;
    return str;
  }

  queue_mutex_.lock();

  // Copy the data.
  data = data_queue_.front();
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  queue_mutex_.unlock();
  return data;
}


bool WorkerSis3302::EventAvailable()
{
  // Check acq reg.
  static uint msg = 0;
  static bool is_event;
  static int count, rc;

  count = 0;
  rc = 0;
  do {

    rc = Read(0x10, msg);
    if (rc != 0) {
      LogError("failed to read event status register");
    }
  } while ((rc != 0) && (count++ < 100));
 
  is_event = !(msg & 0x10000);

  if (is_event && go_time_) {
    // rearm the logic
    uint armit = 1;

    count = 0;
    rc = 0;
    do {

      rc = Write(0x410, armit);
      if (rc != 0) {
        LogError("failed to rearm sampling logic");
      }
    } while ((rc != 0) && (count++ < 100));

    return is_event;
  }

  return false;
}

void WorkerSis3302::GetEvent(sis_3302 &bundle)
{
  using namespace std::chrono;
  int ch, offset, rc, count = 0;

  // Check how long the event is.
  //expected SIS_3302_LN + 8
  
  uint next_sample_address[SIS_3302_CH];
  static uint trace[SIS_3302_CH][SIS_3302_LN / 2];
  static uint timestamp[2];

  for (ch = 0; ch < SIS_3302_CH; ch++) {

    next_sample_address[ch] = 0;

    offset = 0x02000010;
    offset |= (ch >> 1) << 24;
    offset |= (ch & 0x1) << 2;

    count = 0;
    rc = 0;
    do {
      rc = Read(offset, next_sample_address[ch]);
      ++count;
    } while ((rc < 0) && (count < 100));
  }

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  
  rc = Read(0x10000, timestamp[0]);
  if (rc != 0) {
    LogError("failed to read first byte of the device timestamp");
  }

  rc = Read(0x10001, timestamp[1]);
  if (rc != 0) {
    LogError("failed to read second byte of the device timestamp");
  }

  for (ch = 0; ch < SIS_3302_CH; ch++) {

    offset = (0x8 + ch) << 23;
    count = 0;

    do {

      rc = ReadTrace(offset, trace[ch]);
      if (rc != 0) {
        LogError("failed reading trace for channel %i", ch);
      }
    } while ((rc < 0) && (count++ < 100));
  }

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3302_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = timestamp[1] & 0xfff;
    bundle.device_clock[ch] |= (timestamp[1] & 0xfff0000) >> 4;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfffULL) << 24;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfff0000ULL) << 20;

    std::copy((ushort *)trace[ch],
    	      (ushort *)trace[ch] + SIS_3302_LN,
    	      bundle.trace[ch]);
  }
}

} // ::daq
