#include "worker_caen1742.hh"

namespace daq {

WorkerCaen1742::WorkerCaen1742(std::string name, std::string conf) : 
  WorkerVme<caen_1742>(name, conf)
{
  LoadConfig();
}

WorkerCaen1742::~WorkerCaen1742()
{
  thread_live_ = false;
  if (work_thread_.joinable()) {
    try {
      work_thread_.join();
    } catch (std::system_error e) {
      LogError("Encountered race condition joining thread");
    }
  }
}

void WorkerCaen1742::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int rc;
  uint msg = 0;
  std::string tmp;
  char str[256];

  // Get the base address for the device.  Convert from hex.
  tmp = conf.get<std::string>("base_address");
  base_address_ = std::stoul(tmp, nullptr, 0);

  // Software board reset.
  rc = Write(0xef24, msg);
  usleep(300000);

  // Make certain we aren't running
  rc = Read(0x8104, msg);
  if (msg & (0x1 << 2)) {
    LogMessage("Unit was already running on init");
    rc = Read(0x8100, msg);
    msg &= ~(0x1 << 2);
    rc = Write(0x8100, msg);
  }

  // Get the board info.
  rc = Read(0xf034, msg);

  // Board type
  if ((msg & 0xff) == 0x00) {

    LogMessage("Found caen v1742");

  } else if ((msg & 0xff) == 0x01) {

    LogMessage("Found caen vx1742");
  }

  // Check the serial number 
  int sn = 0;
  rc = Read(0xf080, msg);
  sn += (msg & 0xff) << 2;
  
  rc = Read(0xf084, msg);
  sn += (msg & 0xff);
  LogMessage("Serial Number: %i", sn);
  
  // Get the hardware revision numbers.
  uint rev[4];
  
  for (int i = 0; i < 4; ++i) {
    rc = Read(0xf040 + 4*i, msg);
    rev[i] = msg;
  }

  LogMessage("Board Hardware Release %i.%i.%i.%i", 
	     rev[0], rev[1], rev[2], rev[3]);
  
  // Check the temperature.
  for (int i = 0; i < 4; ++i) {

    rc = Read(0x1088 + i*0x100, msg);

    if (msg & ((0x1 << 2) | (0x1 << 8)))
      LogMessage("Unit %i is busy", i);

    rc = Read(0x10A0 + i*0x100, msg);
    LogMessage("DRS4 Chip %i at temperature %i C", i, msg & 0xff);
  }
    
  // Enable external/software triggers.
  rc = Read(0x810c, msg);
  rc = Write(0x810c, msg | (0x3 << 30));

  // Set the group enable mask.
  rc = Read(0x8120, msg);
  rc = Write(0x8120, msg | 0xf);

  // Enable digitization of the triggers using config bit enable register.
  rc = Write(0x8004, 0x1 << 11);

  rc = Read(0x8120, msg);
  LogMessage("Group enable mask reads: %08x", msg);

  // Set the trace length.
  rc = Read(0x8020, msg);
  rc = Write(0x8020, msg & 0xfffffffc); // 1024

  // Set the sampling rate.
  double sampling_rate = conf.get<double>("sampling_rate", 1.0);
  // rc = Read(0x80d8, msg);
  // msg &= 0xfffffffc;
  msg = 0;

  if (sampling_rate < 1.75) {

    msg |= 0x2; // 1.0 Gsps
    LogMessage("Sampling rate set to 1.0 Gsps");

  } else if (sampling_rate >= 1.75 && sampling_rate < 3.75) {

    msg |= 0x1; // 2.5 Gsps
    LogMessage("Sampling rate set to 2.5 Gsps");
    
  } else if (sampling_rate >= 3.75) {

    msg |= 0x0; // 5.0 Gsps
    LogMessage("Sampling rate set to 5.0 Gsps");
  }

  // Write the sampling rate.
  rc = Write(0x80d8, msg);

  // Set "pretrigger" buffer.
  rc = Read(0x8114, msg);
  int pretrigger_delay = conf.get<int>("pretrigger_delay", 35);
  msg &= 0xfffffe00;
  msg |= (uint)(pretrigger_delay / 100.0 * 0x3ff);

  rc = Write(0x8114, msg);

  // if (conf.get<bool>("use_drs4_corrections")) {
  //   // Load and enable DRS4 corrections.
  //   ret = CAEN_DGTZ_LoadDRS4CorrectionData(device_, rate);
  //   ret = CAEN_DGTZ_EnableDRS4Correction(device_);
  // }


  //DAC offsets
  uint ch = 0;
  uint ch_idx = 0;
  uint group_size = CAEN_1742_CH / CAEN_1742_GR;
  for (auto &val : conf.get_child("channel_offset")) {

    // Set group
    int group_idx = ch / group_size;

    // Convert the voltage to a dac value.
    float volts = val.second.get_value<float>();
    uint dac = (uint)((volts / vpp_) * 0xffff + 0x8000);
    
    if (dac < 0x0) dac = 0;
    if (dac > 0xffff) dac = 0xffff;
    
    // Make sure the board isn't busy
    int count = 0;
    while (true) {
      rc = Read(0x1088 + 0x100*group_idx, msg);
      
      if ((rc < 0) || !(msg & 0x84) || (++count > 100)) {
	break;
      }
    }
    
    ch_idx = (ch++) % group_size;
    rc = Write(0x1098 + 0x100*group_idx, dac | (ch_idx << 16));
  }    


  // ret = CAEN_DGTZ_SetGroupFastTriggerDCOffset(device_, 0x10DC, 0x8000);
  // ret = CAEN_DGTZ_SetGroupFastTriggerThreshold(device_, 0x10D4, 0x51C6);

  // // Digitize the fast trigger.
  // ret = CAEN_DGTZ_SetFastTriggerDigitizing(device_, CAEN_DGTZ_ENABLE);

  // // Disable self trigger.
  // ret = CAEN_DGTZ_SetChannelSelfTrigger(device_, 
  // 					CAEN_DGTZ_TRGMODE_DISABLED, 
  // 					0xffff);
  
  // // Channel pulse polarity
  // for (int ch; ch < CAEN_1742_CH; ++ch) {
  //   ret = CAEN_DGTZ_SetChannelPulsePolarity(device_, ch, 
  // 					    CAEN_DGTZ_PulsePolarityPositive);
  // }

  // Start acquiring events.
  int count = 0;
  do {
    usleep(100);
    rc = Read(0x8104, msg);
    ++count;
    LogMessage("Checking if board is ready to acquire");
  } while ((count < 100) && !(msg & 0x100));

  rc = Read(0x8100, msg);
  msg |= (0x1 << 2);
  rc = Write(0x8100, msg);

  usleep(1000);
  // Send a test software trigger and read it out.
  rc = Write(0x8108, msg);

  // Read initial empty event.
  LogMessage("Eating first empty event");
  if (EventAvailable()) {
    caen_1742 bundle;
    GetEvent(bundle);
  }
  LogMessage("LoadConfig finished");

} // LoadConfig

void WorkerCaen1742::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static caen_1742 bundle;
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

  // Stop acquiring events.
  uint rc, msg;
  rc = Read(0x8100, msg);
  msg &= ~(0x1 << 2);
  rc = Write(0x8100, msg);
}

caen_1742 WorkerCaen1742::PopEvent()
{
  static caen_1742 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {

    caen_1742 str;
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


bool WorkerCaen1742::EventAvailable()
{
  // Check acquisition status regsiter.
  uint msg, rc;
  rc = Read(0x8104, msg);

  if (msg & (0x1 << 3)) {
    return true;

  } else {

    return false;
  }
}

void WorkerCaen1742::GetEvent(caen_1742 &bundle)
{
  using namespace std::chrono;
  int ch, offset, rc = 0; 
  char *evtptr = nullptr;
  char str[256];
  uint msg, d, size;
  int sample;

  static std::vector<uint> buffer;
  if (buffer.size() == 0) {
    buffer.reserve((CAEN_1742_CH * CAEN_1742_LN * 4) / 3 + 32);
  }

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  // Get the size of the next event data
  rc = Read(0x814c, msg);
  
  buffer.resize(msg);
  read_trace_len_ = msg;
  LogMessage("Reading trace length: %i", msg);
  //  ReadTraceMblt64(0x0, trace);
  
  // Try reading out word by word
  for (int i = 0; i < read_trace_len_; ++i) {
    rc = Read(0x0, msg);
    buffer[i] = msg;
  }

  LogMessage("First element of event is %08x", buffer[0]);
  // Get the number of current events buffered.
  rc = Read(0x812c, msg);
  LogMessage("%i events in memory", msg);

  // Make sure we aren't getting empty events
  if (buffer.size() < 5) {
    return;
  }

  // Figure out the group mask
  bool grp_mask[CAEN_1742_GR];

  for (int i = 0; i < CAEN_1742_GR; ++i) {
    grp_mask[i] = buffer[1] & (0x1 << i);
  }
  
  // Now unpack the data for each group
  uint header;
  uint chdata[8];
  int start_idx = 4; // Skip main event header
  int stop_idx = 4;

  for (int grp_idx = 0; grp_idx < CAEN_1742_GR; ++grp_idx) {

    // Skip if this group isn't present.
    if (!grp_mask[grp_idx]) {
      LogMessage("Skipping group %i", grp_idx);
      continue;
    }

    // Grab the group header info.
    header = buffer[start_idx++];

    // Check to make sure it is a header
    if ((~header & 0xc00ce000) != 0xc00ce000) {
      LogMessage("Missed header");
    }

    // Calculate the group size.
    int data_size = header & 0xfff;
    int nchannels = CAEN_1742_CH / CAEN_1742_GR;
    bool trg_saved = header & (0x1 << 12);
    
    stop_idx = start_idx + data_size;
    sample = 0;

    LogMessage("start = %i, stop = %i, size = %u\n", start_idx, stop_idx, data_size);
    for (int i = start_idx; i < stop_idx; i += 3) {
      uint ln0 = buffer[i];
      uint ln1 = buffer[i+1];
      uint ln2 = buffer[i+2];
      
      chdata[0] = ln0 & 0xfff;
      chdata[1] = (ln0 >> 12) & 0xfff;
      chdata[2] = ((ln0 >> 24) & 0xff) | ((ln1 & 0xf) << 8);
      chdata[3] = (ln1 >> 4) & 0xfff;
      chdata[4] = (ln1 >> 16) & 0xfff;
      chdata[5] = ((ln1 >> 28) & 0xf) | ((ln2 & 0xff) << 4);
      chdata[6] = (ln2 >> 8) & 0xfff;
      chdata[7] = (ln2 >> 20) & 0xfff;

      for (int j = 0; j < 8; ++j) {
	int ch_idx = j + grp_idx * nchannels;
	bundle.trace[ch_idx][sample] = chdata[j];
      }
      
      ++sample;
    }

    // Update our starting point.
    start_idx = stop_idx;
    sample = 0;

    // Now grab the trigger if it was digitized.
    if (trg_saved) {
      
      LogMessage("Digitizing trigger");
      stop_idx = start_idx + (data_size / 8);

      for (int i = start_idx; i < stop_idx; i += 3) {
	uint ln0 = buffer[i];
	uint ln1 = buffer[i+1];
	uint ln2 = buffer[i+2];
	
	chdata[0] = ln0 & 0xfff;
	chdata[1] = (ln0 >> 12) & 0xfff;
	chdata[2] = ((ln0 >> 24) & 0xff) | ((ln1 & 0xf) << 8);
	chdata[3] = (ln1 >> 4) & 0xfff;
	chdata[4] = (ln1 >> 16) & 0xfff;
	chdata[5] = ((ln1 >> 28) & 0xf) | ((ln2 & 0xff) << 4);
	chdata[6] = (ln2 >> 8) & 0xfff;
	chdata[7] = (ln2 >> 20) & 0xfff;
	
	for (int j = 0; j < 8; ++j) {
	  bundle.trigger[grp_idx][sample++] = chdata[j];
	}
      }
    }

    // Grab the trigger time and increment starting index.
    uint timestamp = buffer[stop_idx++];
    LogMessage("timestamp: 0x%08x\n", timestamp);

    start_idx = stop_idx;
  }
}
  
} // ::daq
