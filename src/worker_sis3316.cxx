#include "worker_sis3316.hh"

namespace daq {

WorkerSis3316::WorkerSis3316(std::string name, std::string conf) : 
  WorkerVme<sis_3316>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3316_CH;
  read_trace_len_ = SIS_3316_LN / 2; // only for vme ReadTrace
  using_bank2 = false;
}

void WorkerSis3316::LoadConfig()
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

  if (rc == 0) {

    LogMessage("SIS3316 Found at 0x%08x", base_address_);

  } else {

    LogError("SIS3316 at 0x%08x could not be found.", base_address_);
  }

  // Reset the device.
  msg = 1;
  rc = Write(0x400, msg);

  if (rc != 0) {
    LogError("error writing sis3316 reset register");
  }

  // Get device ID.
  msg = 0;
  rc = Read(0x4, msg);

  if (rc == 0) {

    LogMessage("ID: %04x, maj rev: %02x, min rev: %02x",
	       msg >> 16, (msg >> 8) & 0xff, msg & 0xff);

  } else {

    LogError("failed to read device ID register");
  }

  // Get device hardware revision.
  msg = 0;
  rc = Read(0x1c, msg);

  if (rc == 0) {

    LogMessage("device hardware version %i", msg & 0xf);

  } else {

    LogError("failed to read device hardware revision");
  }

  // Check the board temperature
  msg = 0;
  rc = Read(0x20, msg);

  if (rc == 0) {

    LogMessage("device internal temperature is %0.2fC", (ushort)(msg) * 0.25);

  } else {

    LogError("failed to read device hardware revision");
  }

  // Read control/status register.
  msg = 0;
  if (conf.get<bool>("enable_ext_lemo")) {
    msg |= (0x1 << 4); // enable external trigger
  }

  if (conf.get<bool>("invert_ext_lemo")) {
    msg |= (0x1 << 5); // invert external trigger
  }

  rc = Write(0x5c, msg);
  if (rc != 0) {
    LogError("failure to write control/status register");
  }

  // Set the DAC offsets by groups of 4 channels
  if (conf.get<bool>("set_voltage_offsets", false)) {

    uint reg;

    for (int ch = 0; ch < SIS_3316_GR; ++ch) {
      
      // todo
      LogWarning("attempting to set voltage offset (not implemented)");
    }
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
    LogError("failed to set the acquisition register");
  }

  msg = 0;
  rc = Read(0x10, msg);

  if (rc != 0) {

    LogError("failed to read the acquisition register");

  } else {
    
    LogMessage("acquisition register set to: 0x%08x", msg);
  }

  // Need to enable triggers per channel also, I think.
  for (int ch = 0; ch < SIS_3316_GR; ++ch) {
    
    uint reg = 0x10 + 0x1000 * (ch + 1);

    rc = Read(reg, msg);
    if (rc != 0) {
      LogError("failure reading event config for channel group %i", ch + 1);
    }
    
    if (conf.get<bool>("enable_ext_lemo", true)) {
      msg |= 0x1 << 3;
      msg |= 0x1 << 11;
      msg |= 0x1 << 19;
      msg |= 0x1 << 27;
    }

    rc = Write(reg, msg);
    if (rc != 0) {
	LogError("failure writing event config for channel group %i", ch + 1);
    }
  }

  // Set the start delay.
  msg = conf.get<int>("start_delay", 0);
  rc = Write(0x14, msg);
  
  if (rc != 0) {
    LogError("failure to set the start_delay");
  }

  // Set the stop delay.
  msg = conf.get<int>("stop_delay", 0);
  rc = Write(0x18, msg);

  if (rc != 0) {
    LogError("failure to set the stop_delay");
  }

  // Read event configure register.
  msg = 0;
  rc = Read(0x02000000, msg);
  
  if (rc != 0) {
    LogError("failure to read event configuration register");
  }

  // Set event configure register with changes
  if (conf.get<bool>("enable_event_length_stop", true))
    msg = 0x1 << 5; // enable event length stop trigger

  rc = Write(0x01000000, msg);

  if (rc != 0) {
    LogError("failed to enable event length stops");
  }
  
  // Set event length register per ADC.
  msg = (SIS_3316_LN) & 0x1fffffe; 
  for (int ch = 0; ch < SIS_3316_GR; ++ch) {

    uint reg = 0x98 + 0x1000 * (ch + 1);

    rc = Write(reg, msg);
    
    if (rc != 0) {
      LogError("failed to set the extend raw buffer sample length.");
    }
  }

  // Set the pre-trigger buffer length for each channel.
  msg = std::stoi(conf.get<string>("pretrigger_samples", "0x0"), nullptr, 0);

  for (int ch = 0; ch < SIS_3316_GR; ++ch) {
    
    uint reg = 0x28 + 0x1000 * (ch + 1);
    msg &= 0x1ffe;
    
    rc = Write(0x01000060, msg);
    if (rc != 0) {
      LogError("failure setting pre-trigger for channel group %i", ch + 1);
    }
  }

  // Arm bank1 to start.
  msg = 1;
  rc = Write(0x420, msg);
  
  if (rc != 0) {
    LogError("failed to arm logic on bank 1");
  }
} // LoadConfig

void WorkerSis3316::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static sis_3316 bundle;
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

sis_3316 WorkerSis3316::PopEvent()
{
  static sis_3316 data;

  if (data_queue_.empty()) {
    sis_3316 str;
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


bool WorkerSis3316::EventAvailable()
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

  if (is_event && go_time_) {
    // rearm the logic
    uint armit = 1;

    count = 0;
    rc = 0;
    do {
      if (using_bank2) {

	rc = Write(0x420, armit);
	using_bank2 = false;

      } else {

	rc = Write(0x424, armit);
	using_bank2 = true;
      }	

      ++count;
    } while ((rc < 0) && (count < 100));

    return is_event;
  }

  return false;
}

void WorkerSis3316::GetEvent(sis_3316 &bundle)
{
  using namespace std::chrono;
  int ch, rc, count = 0;
  uint trace_addr, offset;

  // Check how long the event is.
  uint next_sample_address[SIS_3316_CH];
  static uint trace[SIS_3316_CH][SIS_3316_LN / 2];
  static uint timestamp[2];

  for (ch = 0; ch < SIS_3316_CH; ch++) {

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

  // Get the system time.
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  // Get the device timestamp.
  Read(0x10000, timestamp[0]);
  Read(0x10001, timestamp[1]);

  // Now the traces.
  for (ch = 0; ch < SIS_3316_CH; ch++) {
    
    offset = 0x1000 * ((ch / SIS_3316_GR) + 1);
    offset += 0x110 + 0x4 * (ch % SIS_3316_GR);

    rc = Read(offset, trace_addr);
    if (rc != 0) {
      LogError("couldn't read address of next trace");
    }

    count = 0;
    rc = 0;
    LogMessage("attempting to read trace at 0x%08x", trace_addr);
    do {
      rc = ReadTrace(trace_addr, trace[ch]);
      ++count;
    } while ((rc < 0) && (count < 100));
  }

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3316_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = timestamp[1] & 0xfff;
    bundle.device_clock[ch] |= (timestamp[1] & 0xfff0000) >> 4;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfffULL) << 24;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfff0000ULL) << 20;

    std::copy((ushort *)trace[ch],
    	      (ushort *)trace[ch] + SIS_3316_LN,
    	      bundle.trace[ch]);
  }
}

} // ::daq
