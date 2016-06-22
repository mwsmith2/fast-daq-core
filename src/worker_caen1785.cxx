#include "worker_caen1785.hh"

namespace daq {

WorkerCaen1785::WorkerCaen1785(std::string name, std::string conf) : WorkerVme<caen_1785>(name, conf)
{
  LoadConfig();

  num_ch_ = CAEN_1785_CH;
  read_trace_len_ = 1;
}

void WorkerCaen1785::LoadConfig()
{
  read_low_adc_ = conf_.get<bool>("read_low_adc", false);

  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoi(conf_.get<std::string>("base_address"), nullptr, 0);

  int rc;
  uint msg = 0;
  ushort msg_16 = 0;

  rc = Read(0x0, msg);
  if (rc != 0) {

    LogError("could not find module");

  } else {

    LogMessage("CAEN 1785 found at 0x%08x", base_address_);
  }

  // Reset the device.
  rc = Write16(0x1006, 0x80); // J
  if (rc != 0) {
    LogError("failed to reset device, j register");
  }

  rc = Write16(0x1008, 0x80); // and K
  if (rc != 0) {
    LogError("failed to reset device, k register");
  }

  rc = Read16(0x1000, msg_16);
  if (rc != 0) {

    LogError("failed to read firmware revision");

  } else {

    LogMessage("Firmware Revision: %i.%i", (msg_16 >> 8) & 0xff, msg_16& 0xff);
  }

  // Disable suppressors.
  //  Write16(0x1032, 0x18);

  // Reset the data on the device.
  ClearData();
  LogMessage("CAEN 1785 data cleared");

} // LoadConfig

void WorkerCaen1785::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static caen_1785 bundle;
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

caen_1785 WorkerCaen1785::PopEvent()
{
  static caen_1785 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {
    queue_mutex_.unlock();

    caen_1785 str;
    return str;

  } else if (!data_queue_.empty()) {

    // Copy and pop the data.
    data = data_queue_.front();
    data_queue_.pop();

    // Check if this is that last event.
    if (data_queue_.size() == 0) has_event_ = false;

    queue_mutex_.unlock();
    return data;
  }
}


bool WorkerCaen1785::EventAvailable()
{
  // Check acq reg.
  static ushort msg_16= 0;
  static uint msg, rc;
  static bool is_event;

  // Check if the device has data.
  rc = Read16(0x100E, msg_16);
  if (rc != 0) {
    LogError("failed checking device status");
  }

  is_event = (msg_16 & 0x1);

  // Check to make sure the buffer isn't empty.
  rc = Read16(0x1022, msg_16);
  if (rc != 0) {
    LogError("failed checking for empty buffer");
  }

  is_event &= !(msg & 0x2);

  return is_event;
}

void WorkerCaen1785::GetEvent(caen_1785 &bundle)
{
  using namespace std::chrono;
  int offset = 0x0;
  uint rc = 0, ch = 0, data = 0;

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();

  // Read the data for each high value.
  while ((((data >> 24) & 0x7) != 0x6) || (((data >> 24) &0x7) != 0x4)) {

    if (offset >= 0x1000) break;

    rc = Read(offset, data);
    if (rc != 0) {
      LogError("failed reading event data at addr: 0x%08x", offset);
    }

    offset += 4;

    if (((data >> 24) & 0x7) == 0x0) {

      if (((data >> 17) & 0x1) && read_low_adc_) {

	// Skip high values.
      	continue;

      } else if (!((data >> 17) & 0x1) && !read_low_adc_) {

	// Skip low values.
      	continue;
      }

      bundle.device_clock[ch] = 0; // No device time
      bundle.value[ch] = (data & 0xfff);

      if (ch == 7) {
	// Increment event register and exit.
	rc = Write16(0x1028, 0x0);
        if (rc != 0) {
          LogError("failure incrementing event register");
        }

	break;
      }

      // Device in order 0, 4, 1, 5, 2, 6, 3, 7.
      ch = (ch > 3) * (ch - 3) + (ch < 4) * (ch + 4);
    }
  }
}

} // ::daq
