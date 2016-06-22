#include "worker_caen6742.hh"

namespace daq {

WorkerCaen6742::WorkerCaen6742(std::string name, std::string conf) :
  WorkerBase<caen_6742>(name, conf)
{
  buffer_ = nullptr;
  event_ = nullptr;

  LoadConfig();
}

WorkerCaen6742::~WorkerCaen6742()
{
  thread_live_ = false;
  if (work_thread_.joinable()) {
    try {
      work_thread_.join();
    } catch (std::system_error e) {
      LogError("Encountered race condition joining thread");
    }
  }

  //CAEN_DGTZ_Reset(device_);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer_);
  CAEN_DGTZ_CloseDigitizer(device_);
}

void WorkerCaen6742::LoadConfig()
{
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int rc;
  uint msg = 0;

  int device_id = conf_.get<int>("device_id");

  // Reset the device.
  rc = CAEN_DGTZ_Reset(device_);
  if (rc != 0) {
    LogError("failed to reset device");
  }

  // Get the board info.
  rc = CAEN_DGTZ_GetInfo(device_, &board_info_);
  if (rc != 0) {

    LogError("failed to get device info");

  } else {

    LogMessage("Found caen %s.", board_info_.ModelName);
    LogMessage("Serial Number: %i.", board_info_.SerialNumber);
    LogMessage("ROC FPGA Release is %s", board_info_.ROC_FirmwareRel);
    LogMessage("AMC FPGA Release is %s", board_info_.AMC_FirmwareRel);
  }

  // Set the trace length.
  rc = CAEN_DGTZ_SetRecordLength(device_, CAEN_6742_LN);
  if (rc != 0) {
    LogError("failed to set trace length");
  }

  // Set "pretrigger".
  int pretrigger_delay = conf_.get<int>("pretrigger_delay", 35);
  rc = CAEN_DGTZ_SetPostTriggerSize(device_, pretrigger_delay);
  if (rc != 0) {
    LogError("failed to set pretrigger buffer");
  }

  // Set the sampling rate.
  CAEN_DGTZ_DRS4Frequency_t rate;
  double sampling_rate = conf_.get<double>("sampling_rate", 1.0);

  if (sampling_rate > 3.75) {

    LogMessage("Setting sampling rate to 5.0GHz.");
    rate = CAEN_DGTZ_DRS4_5GHz;

  } else if (sampling_rate <= 3.75 && sampling_rate >= 2.25) {

    LogMessage("Setting sampling rate to 2.5GHz.");
    rate = CAEN_DGTZ_DRS4_2_5GHz;

  } else {

    LogMessage("Setting sampling rate to 1.0GHz.");
    rate = CAEN_DGTZ_DRS4_1GHz;
  }

  rc = CAEN_DGTZ_SetDRS4SamplingFrequency(device_, rate);
  if (rc != 0) {
    LogError("failed to set sampling frequency");
  }

  // Load and enable DRS4 corrections.
  if (conf_.get<bool>("use_drs4_corrections")) {

    rc = CAEN_DGTZ_LoadDRS4CorrectionData(device_, rate);
    if (rc != 0) {
      LogError("failed to load DRS4 correction data");
    }

    rc = CAEN_DGTZ_EnableDRS4Correction(device_);
    if (rc != 0) {
      LogError("failed to enable DRS4 corrections");
    }
  }

  // Set the channel enable mask.
  rc = CAEN_DGTZ_SetGroupEnableMask(device_, 0x3); // all on
  if (rc != 0) {
    LogError("failed to set group enable mask");
  }

  uint ch = 0;
  //DAC offsets
  for (auto &val : conf_.get_child("channel_offset")) {

    float volts = val.second.get_value<float>();
    int dac = (int)((volts / vpp_) * 0xffff + 0x8000);

    if (dac < 0x0) dac = 0;
    if (dac > 0xffff) dac = 0xffff;

    rc = CAEN_DGTZ_SetChannelDCOffset(device_, ch++, dac);
    if (rc != 0) {
      LogError("failed to set DC offset for channel %i", ch-1);
    }
  }

  // Enable external trigger.
  //  rc = CAEN_DGTZ_SetExtTriggerInputMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  // Enable fast trigger - NIM
  rc = CAEN_DGTZ_SetFastTriggerMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  if (rc != 0) {
    LogError("failed to set trigger mode to acquire only");
  }

  rc = CAEN_DGTZ_SetGroupFastTriggerDCOffset(device_, 0x10DC, 0x8000);
  if (rc != 0) {
    LogError("failed to set fast trigger DC offset");
  }

  rc = CAEN_DGTZ_SetGroupFastTriggerThreshold(device_, 0x10D4, 0x51C6);
  if (rc != 0) {
    LogError("failed to adjust fast trigger threshold");
  }

  // Digitize the fast trigger.
  rc = CAEN_DGTZ_SetFastTriggerDigitizing(device_, CAEN_DGTZ_ENABLE);
  if (rc != 0) {
    LogError("failed to enable digitization of the fast trigger");
  }

  // Disable self trigger.
  rc = CAEN_DGTZ_SetChannelSelfTrigger(device_,
					CAEN_DGTZ_TRGMODE_DISABLED,
					0xffff);
  if (rc != 0) {
    LogError("failed to disable self-triggering");
  }

  // Channel pulse polarity
  for (int ch; ch < CAEN_6742_CH; ++ch) {
    rc = CAEN_DGTZ_SetChannelPulsePolarity(device_, ch,
					    CAEN_DGTZ_PulsePolarityPositive);
    if (rc != 0) {
      LogError("failed to set pulse polarity for channel %i", ch);
    }
  }

  // Set the acquisition mode.
  rc = CAEN_DGTZ_SetAcquisitionMode(device_, CAEN_DGTZ_SW_CONTROLLED);
  if (rc != 0) {
    LogError("failed to set acquisition mode to software controlled");
  }

  // Set max events to 1 our purposes.
  rc = CAEN_DGTZ_SetMaxNumEventsBLT(device_, 1);
  if (rc != 0) {
    LogError("failed to set max number of events to 1");
  }

  // Allocated the readout buffer.
  rc = CAEN_DGTZ_MallocReadoutBuffer(device_, &buffer_, &size_);
  if (rc != 0) {
    LogError("failed to allocate readout buffer");
  }

} // LoadConfig

void WorkerCaen6742::WorkLoop()
{
  int rc = 0;

  rc = CAEN_DGTZ_AllocateEvent(device_, (void **)&event_);
  if (rc != 0) {
    LogError("failed to allocate event buffer");
  }

  rc = CAEN_DGTZ_SWStartAcquisition(device_);
  if (rc != 0) {
    LogError("failed to begin software data acquisition");
  }

  t0_ = std::chrono::high_resolution_clock::now();

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
	usleep(daq::short_sleep);
      }
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }

  rc = CAEN_DGTZ_SWStopAcquisition(device_);
  if (rc != 0) {
    LogError("failed to end software data acquisition");
  }

  rc = CAEN_DGTZ_FreeEvent(device_, (void **)&event_);
  if (rc != 0) {
    LogError("failed to free event buffer");
  }
}

caen_6742 WorkerCaen6742::PopEvent()
{
  static caen_6742 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {
    queue_mutex_.unlock();

    caen_6742 str;
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


bool WorkerCaen6742::EventAvailable()
{
  // Check acq reg.
  uint num_events = 0, rc = 0;

  rc = CAEN_DGTZ_ReadData(device_,
                          CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                          buffer_,
                          &bsize_);
  if (rc != 0) {
    LogError("failed to readout data");
  }

  rc = CAEN_DGTZ_GetNumEvents(device_, buffer_, bsize_, &num_events);

  if (rc != 0) {
    LogError("failed to read number of events in data buffer");
  }

  if (num_events > 0) {
    return true;

  } else {
    return false;

  }
}

void WorkerCaen6742::GetEvent(caen_6742 &bundle)
{
  using namespace std::chrono;
  int ch, offset, rc = 0;
  char *evtptr = nullptr;

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();

  // Get the event data
  rc = CAEN_DGTZ_GetEventInfo(device_,
                              buffer_,
                              bsize_,
                              0,
                              &event_info_,
                              &evtptr);
  if (rc != 0) {
    LogError("failed to retrieve event info");
  }

  rc = CAEN_DGTZ_DecodeEvent(device_, evtptr, (void **)&event_);
  if (rc != 0) {
    LogError("failed to decode event");
  }

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
      std::cout << "Copying event." << std::endl;
      std::copy(event_->DataGroup[gr].DataChannel[ch],
		event_->DataGroup[gr].DataChannel[ch] + len,
		bundle.trace[ch_idx]);
    }
  }
}

} // ::daq
