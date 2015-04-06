#include "daq_worker_caen6742.hh"

namespace daq {

DaqWorkerCaen6742::DaqWorkerCaen6742(string name, string conf) : DaqWorkerBase<caen_6742>(name, conf)
{
  buffer_ = nullptr;
  event_ = nullptr;

  LoadConfig();
}

DaqWorkerCaen6742::~DaqWorkerCaen6742()
{
  thread_live_ = false;
  if (work_thread_.joinable()) {
    work_thread_.join();
  }

  //CAEN_DGTZ_Reset(device_);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer_);
  CAEN_DGTZ_CloseDigitizer(device_);
}

void DaqWorkerCaen6742::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int ret;
  uint msg = 0;

  int device_id = conf.get<int>("device_id");

  // Reset the device.
  ret = CAEN_DGTZ_Reset(device_);

  // Get the board info.
  ret = CAEN_DGTZ_GetInfo(device_, &board_info_);
  printf("\nFound caen %s.\n", board_info_.ModelName);
  printf("\nSerial Number: %i.\n", board_info_.SerialNumber);
  printf("\tROC FPGA Release is %s\n", board_info_.ROC_FirmwareRel);
  printf("\tAMC FPGA Release is %s\n", board_info_.AMC_FirmwareRel);
  
  // Set the trace length.
  ret = CAEN_DGTZ_SetRecordLength(device_, CAEN_6742_LN);

  // Set "pretrigger".
  int pretrigger_delay = conf.get<int>("pretrigger_delay", 35);
  ret = CAEN_DGTZ_SetPostTriggerSize(device_, pretrigger_delay);

  // Set the sampling rate.
  CAEN_DGTZ_DRS4Frequency_t rate;
  double sampling_rate = conf.get<double>("sampling_rate", 1.0);

  if (sampling_rate > 3.75) {

    printf("Setting sampling rate to 5.0GHz.\n");
    rate = CAEN_DGTZ_DRS4_5GHz;

  } else if (sampling_rate <= 3.75 && sampling_rate >= 2.25) {

    printf("Setting sampling rate to 2.5GHz.\n");
    rate = CAEN_DGTZ_DRS4_2_5GHz;

  } else {

    printf("Setting sampling rate to 1.0GHz.\n");
    rate = CAEN_DGTZ_DRS4_1GHz;
  }  

  ret = CAEN_DGTZ_SetDRS4SamplingFrequency(device_, rate);

  if (conf.get<bool>("use_drs4_corrections")) {
    // Load and enable DRS4 corrections.
    ret = CAEN_DGTZ_LoadDRS4CorrectionData(device_, rate);
    ret = CAEN_DGTZ_EnableDRS4Correction(device_);
  }

  // Set the channel enable mask.
  ret = CAEN_DGTZ_SetGroupEnableMask(device_, 0x3); // all on

  uint ch = 0;
  //DAC offsets
  for (auto &val : conf.get_child("channel_offset")) {

    float volts = val.second.get_value<float>();
    int dac = (int)((volts / vpp_) * 0xffff + 0x8000);

    if (dac < 0x0) dac = 0;
    if (dac > 0xffff) dac = 0xffff;
    
    CAEN_DGTZ_SetChannelDCOffset(device_, ch++, dac);
  }    

  // Enable external trigger.
  //  ret = CAEN_DGTZ_SetExtTriggerInputMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  // Enable fast trigger - NIM
  ret = CAEN_DGTZ_SetFastTriggerMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  ret = CAEN_DGTZ_SetGroupFastTriggerDCOffset(device_, 0x10DC, 0x8000);
  ret = CAEN_DGTZ_SetGroupFastTriggerThreshold(device_, 0x10D4, 0x51C6);

  // Digitize the fast trigger.
  ret = CAEN_DGTZ_SetFastTriggerDigitizing(device_, CAEN_DGTZ_ENABLE);

  // Disable self trigger.
  ret = CAEN_DGTZ_SetChannelSelfTrigger(device_, 
					CAEN_DGTZ_TRGMODE_DISABLED, 
					0xffff);
  
  // Channel pulse polarity
  for (int ch; ch < CAEN_6742_CH; ++ch) {
    ret = CAEN_DGTZ_SetChannelPulsePolarity(device_, ch, 
					    CAEN_DGTZ_PulsePolarityPositive);
  }

  // Set the acquisition mode.
  ret = CAEN_DGTZ_SetAcquisitionMode(device_, CAEN_DGTZ_SW_CONTROLLED);

  // Set max events to 1 our purposes.
  ret = CAEN_DGTZ_SetMaxNumEventsBLT(device_, 1);
  
  // Allocated the readout buffer.
  ret = CAEN_DGTZ_MallocReadoutBuffer(device_, &buffer_, &size_);
  
} // LoadConfig

void DaqWorkerCaen6742::WorkLoop()
{
  int ret = CAEN_DGTZ_AllocateEvent(device_, (void **)&event_);
  ret = CAEN_DGTZ_SWStartAcquisition(device_);
  t0_ = high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static caen_6742 bundle;
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

  ret = CAEN_DGTZ_SWStopAcquisition(device_);
  ret = CAEN_DGTZ_FreeEvent(device_, (void **)&event_);
}

caen_6742 DaqWorkerCaen6742::PopEvent()
{
  static caen_6742 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {

    caen_6742 str;
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


bool DaqWorkerCaen6742::EventAvailable()
{
  // Check acq reg.
  uint num_events = 0;

  CAEN_DGTZ_ReadData(device_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer_, &bsize_);

  CAEN_DGTZ_GetNumEvents(device_, buffer_, bsize_, &num_events);

  if (num_events > 0) {
    return true;

  } else {
    return false;

  }
}

void DaqWorkerCaen6742::GetEvent(caen_6742 &bundle)
{
  int ch, offset, ret = 0;
  char *evtptr = nullptr;

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  // Get the event data
  ret = CAEN_DGTZ_GetEventInfo(device_, buffer_, bsize_, 0, &event_info_, &evtptr);
  ret = CAEN_DGTZ_DecodeEvent(device_, evtptr, (void **)&event_);

  int gr, idx, ch_idx, len;
  for (gr = 0; gr < CAEN_6742_GR; ++gr) {
    for (ch = 0; ch < CAEN_6742_CH / CAEN_6742_GR; ++ch) {

      // Set the channels to fill as group0..group1..tr0..tr1.
      if (ch < CAEN_6742_CH / CAEN_6742_GR - 1) {
	ch_idx = ch + gr * (CAEN_6742_CH / CAEN_6742_GR - 1);
      } else {
	ch_idx = CAEN_6742_CH - 2 + gr;
      }

      int len = event_->DataGroup[gr].ChSize[ch];
      bundle.device_clock[ch_idx] = event_->DataGroup[gr].TriggerTimeTag;
      cout << "Copying event." << endl;
      std::copy(event_->DataGroup[gr].DataChannel[ch],
		event_->DataGroup[gr].DataChannel[ch] + len,
		bundle.trace[ch_idx]);
    }
  }
}
  
} // ::daq
