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
  sampling_setting_ = 0;

  if (sampling_rate < 1.75) {

    sampling_setting_ |= 0x2; // 1.0 Gsps
    LogMessage("Sampling rate set to 1.0 Gsps");

  } else if (sampling_rate >= 1.75 && sampling_rate < 3.75) {

    sampling_setting_ |= 0x1; // 2.5 Gsps
    LogMessage("Sampling rate set to 2.5 Gsps");
    
  } else if (sampling_rate >= 3.75) {

    sampling_setting_ |= 0x0; // 5.0 Gsps
    LogMessage("Sampling rate set to 5.0 Gsps");
  }

  // Write the sampling rate.
  rc = Write(0x80d8, sampling_setting_);

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

  // WriteCorrectionDataCsv();

  // Enable external/software triggers.
  rc = Read(0x810c, msg);
  rc = Write(0x810c, msg | (0x3 << 30));

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
  uint msg, d, size;
  int sample;
  std::vector<uint> startcells(4, 0);

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
      LogWarning("Skipping group %i", grp_idx);
      continue;
    }

    // Grab the group header info.
    header = buffer[start_idx++];

    // Check to make sure it is a header
    if ((~header & 0xc00ce000) != 0xc00ce000) {
      LogWarning("Missed header");
    }

    // Calculate the group size.
    int data_size = header & 0xfff;
    int nchannels = CAEN_1742_CH / CAEN_1742_GR;
    bool trg_saved = header & (0x1 << 12);
    startcells[grp_idx] = (header >> 20) & 0x3ff;
    
    stop_idx = start_idx + data_size;
    sample = 0;

    LogDebug("start = %i, stop = %i, size = %u", start_idx, stop_idx, data_size);
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

  if (true) {
    ApplyDataCorrection(bundle, startcells);
  }
}


// This function does the caen corrections directly as they do.
int WorkerCaen1742::ApplyDataCorrection(caen_1742 &data, 
					const std::vector<uint> &startcells)
{
  static bool correction_loaded = false;
  static drs_correction table;

  if (!correction_loaded) {
    GetCorrectionData(table);
  }

  CellCorrection(data, table, startcells);
  //  PeakCorrection(data, table);

  return 0;
}


int WorkerCaen1742::CellCorrection(caen_1742 &data, 
				   const drs_correction &table,
				   const std::vector<uint> &startcells)
{
  LogDebug("running cell correction");

  uint i, j, k;
  short startcell;
  uint nchannels = CAEN_1742_CH / CAEN_1742_GR;
  
  for (i = 0; i < CAEN_1742_CH; ++i) {

    startcell = startcells[i / nchannels];

    for (j = 0; j < CAEN_1742_LN; ++j) {

      data.trace[i][j] -= table.cell[i][(startcell + j) % 1024];
      data.trace[i][j] -= table.nsample[i][j];
    }
  }

  return 0;
}


// todo: make this human readable.
int WorkerCaen1742::PeakCorrection(caen_1742 &data, const drs_correction &table)
{
  LogDebug("running drs peak correction");
  int offset;
  uint i, j;

  for (i = 0; i < CAEN_1742_CH; ++i) {
    data.trace[i][0] = data.trace[i][1];
  }
  
  for (i = 0; i < CAEN_1742_LN; ++i) {

    offset = 0;

    for (j = 0; j < CAEN_1742_CH; ++j) {

      LogDebug("peak correction on cell[%i][%i]", i, j);

      if (i == 1) {

	if (data.trace[j][2] - data.trace[j][1] > 30) {
	  
	  offset++;
	  
	} else {
	  
	  if ((data.trace[j][3] - data.trace[j][1] > 30) &&
	      (data.trace[j][3] - data.trace[j][2] > 30)) {
	    
	    offset++;
	  }
	}

      } else {

	if ((i == CAEN_1742_CH - 1) && 
	    (data.trace[j][i-1] - data.trace[j][i] > 30)) {
	  offset++;

	} else {
	  
	  if (data.trace[j][i-1] - data.trace[j][i] > 30) {

	    if (data.trace[j][i+1] - data.trace[j][i] > 30) {
		
	      offset++;

	    } else {
	      
	      if ((i == CAEN_1742_CH - 2) || 
		  (data.trace[j][i+2] - data.trace[j][i] > 30)) {
		
		offset++;
	      }
	    }
	  }
	}
      }
    }
    
    if (offset == 8) {
      
      for (j = 0; j < (CAEN_1742_CH / CAEN_1742_GR); ++j) {

	if (i == 1) {
	  
	  if (data.trace[j][2] - data.trace[j][1] > 30) {
	    
	    data.trace[j][0] = data.trace[j][2];
	    data.trace[j][1] = data.trace[j][2];
	    
	  } else {
	    
	    data.trace[j][0] = data.trace[j][3];
	    data.trace[j][1] = data.trace[j][3];
	    data.trace[j][2] = data.trace[j][3];
	  }
	  
	} else {
	  
	  if (i == CAEN_1742_CH - 1) {
	    
	    data.trace[j][CAEN_1742_CH - 1] = data.trace[j][CAEN_1742_CH - 2];
	    
	  } else {
	    
	    if (data.trace[j][i + 1] - data.trace[j][i] > 30) {
	      
	      data.trace[j][i] = 0.5 * (data.trace[j][i+1] + data.trace[j][i-1]);
	      
	    } else {
	      
	      if (i == CAEN_1742_CH - 2) {
		
		data.trace[j][i] = data.trace[j][i-1];
		data.trace[j][i+1] = data.trace[j][i-1];
		
	      } else {
		
		data.trace[j][i] = 0.5 * (data.trace[j][i+2] + data.trace[j][i-1]);
		data.trace[j][i+1] = 0.5 * (data.trace[j][i+2] + data.trace[j][i-1]);
	      }
	    }
	  }
	}
      }
    } // j
  } // i

  return 0;
}


int WorkerCaen1742::GetChannelCorrectionData(uint ch, drs_correction &table)
{
  int rc = 0;
  uint32_t d32 = 0;
  uint16_t d16 = 0;

  uint32_t group = 0;
  uint32_t pagenum = 0;
  int start = 0;
  int chunk = 256;
  int last = 0;
  short cell = 0;

  std::vector<int8_t> vec;

  // Set the page index
  group = ch / (CAEN_1742_CH / CAEN_1742_GR);
  pagenum = (group % 2) ? 0xc00 : 0x800;
  pagenum |= (sampling_setting_) << 8;
  pagenum |= (ch % (CAEN_1742_CH / CAEN_1742_GR)) << 2;
  
  LogMessage("channel %i pagenum = 0x%08x", ch, pagenum);

  // Get the cell corrections
  for (int i = 0; i < 4; ++i) {

    ReadFlashPage(group, pagenum, vec);

    // Peak correction if needed.
    last = chunk;
    for (int j = start; j < start + chunk; ++j) {
      
      // No correction.
      if (vec[j - start] != 0x7f) {
	
	table.cell[ch][j] = vec[j - start];

      } else {
	
	cell = (short)((vec[last + 1] << 8) | vec[last]);
	
	if (cell == 0) {
	  
	  table.cell[ch][j] = vec[j - start];

	} else {
	
	  table.cell[ch][j] = cell;
	}

	last += 2;

	if (last > 263) {
	  last = 256; // This should never happen
	}
      }
    } // Peak correction

    // Increment for next round.
    start += chunk;
    pagenum++;
  }

  // Now get the nsample correction data.
  start = 0;

  // Calculate new pagenum via magic numbers
  pagenum &= 0xf00;
  pagenum |= 0x40;
  pagenum |= (ch % (CAEN_1742_CH / CAEN_1742_GR)) << 2;

  for (int i = 0; i < 4; ++i) {

    ReadFlashPage(group, pagenum, vec);
    
    for (int j = start; j < start + chunk; ++j) {
      
      table.nsample[ch][j] = vec[j - start];

    }

    start += chunk;
    pagenum++;
  }

  // Read the time correction if it's the first channel in the group.
  if (ch % (CAEN_1742_LN / CAEN_1742_GR) == 0) {
    
    pagenum &= 0xf00;
    pagenum |= 0xa0;

    start = 0;

    for (int i = 0; i < 16; ++i) {
      
      rc = ReadFlashPage(group, pagenum, vec);

      std::copy((float *)&vec[0], 
		(float *)&vec[chunk - 4], 
		&table.time[group][start]);

      start += chunk / 4;
      pagenum++;
    }
  }
}

int WorkerCaen1742::ReadFlashPage(uint32_t group, 
				  uint32_t pagenum, 
				  std::vector<int8_t> &page)
{
  LogMessage("reading flash page 0x%08x for group %i", pagenum, group);

  // Some basic variables
  int rc = 0;
  uint32_t d32 = 0;
  uint16_t d16 = 0;

  // Set some vme registers for convenience.
  uint32_t gr_status =  0x1088 | (group << 8);
  uint32_t gr_sel_flash = 0x10cc | (group << 8);
  uint32_t gr_flash = 0x10d0 | (group << 8);
  uint32_t flash_addr = pagenum << 9;

  page.resize(0);   // clear data
  page.resize(264); // magic resize

  rc = Read16(gr_status, d16);
  LogMessage("group %i status is 0x%04x", group, d16);

  // Enable the flash memory and tell it to read the main memory page.
  rc = Write16(gr_sel_flash, 0x1);
  rc = Write16(gr_flash, 0xd2);
  rc = Write16(gr_flash, (flash_addr >> 16) & 0xff);
  rc = Write16(gr_flash, (flash_addr >> 8) & 0xff);
  rc = Write16(gr_flash, (flash_addr) & 0xff);

  // Requires for more writes for no apparent reason.
  rc = Write16(gr_flash, 0x0);
  rc = Write16(gr_flash, 0x0);
  rc = Write16(gr_flash, 0x0);
  rc = Write16(gr_flash, 0x0);

  // Now read the data into the output vector.
  for (int i = 0; i < 264; ++i) {

    rc = Read(gr_flash, d32);
    page[i] = (int8_t)(d32 & 0xff);

    if (i == 0) {
      LogMessage("first value in pagenum 0x%08x is %i", pagenum, d32 & 0xff);
    }

    rc = Read(gr_status, d32);
  }

  // Disable the flash memory
  rc = Write16(gr_sel_flash, 0x0);

  return 0;
}


// This ended up being a silly division of labor, 
// might merge with GetChannelCorrectionData.
int WorkerCaen1742::GetCorrectionData(drs_correction &table) 
{
  for (uint ch = 0; ch < CAEN_1742_CH; ++ch) {
    GetChannelCorrectionData(ch, table);
  }
}


int WorkerCaen1742::WriteCorrectionDataCsv()
{
  drs_correction table;
  GetCorrectionData(table);

  // unfinished, just printing for now
  std::ofstream out;
  out.open("correction_table.csv");
  
  for (int i = -1; i < 1024; ++i) {

    // Cell corrections
    for (int j = 0; j < CAEN_1742_CH; ++j) {
      
      if (i == -1) {

	out << "cell_ch_" << j;

      }	else {

	out << table.cell[j][i];
	
      }

      out << ", ";

    } // cell

    // nsample corrections
    for (int j = 0; j < CAEN_1742_CH; ++j) {
      
      if (i == -1) {

	out << "nsample_ch_" << j;

      }	else {

	out << (int)table.nsample[j][i];
	
      }

      out << ", ";

    } // nsample

    // timing corrections
    for (int j = 0; j < CAEN_1742_GR; ++j) {
      
      if (i == -1) {

	out << "time_gr_" << i + 1;

      }	else {

	out << table.cell[j][i];
	
      }

      if (j != CAEN_1742_GR - 1) {

	out << ", ";

      } else {
	
	out << std::endl;

      }
    } // time
  } // i
}



} // ::daq
