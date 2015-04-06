#include "daq_worker_caen1785.hh"

namespace daq {

DaqWorkerCaen1785::DaqWorkerCaen1785(string name, string conf) : DaqWorkerVme<caen_1785>(name, conf)
{
  LoadConfig();

  num_ch_ = CAEN_1785_CH;
  read_trace_len_ = 1;
}

void DaqWorkerCaen1785::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  read_low_adc_ = conf.get<bool>("read_low_adc", false);

  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoi(conf.get<string>("base_address"), nullptr, 0);
  
  int ret;
  uint msg = 0;
  ushort msg_16 = 0;

  Read(0x0, msg);
  printf("CAEN 1785 found at 0x%08x\n", base_address_);

  // Reset the device.
  Write16(0x1006, 0x80); // J
  Write16(0x1008, 0x80); // and K

  Read16(0x1000, msg_16);
  printf("Firmware Revision: %i.%i\n", (msg_16 >> 8) & 0xff, msg_16& 0xff); 

  // Disable suppressors.
  //  Write16(0x1032, 0x18);

  // Reset the data on the device.
  ClearData();
  printf("CAEN 1785 data cleared.\n");

} // LoadConfig

void DaqWorkerCaen1785::WorkLoop()
{
  t0_ = high_resolution_clock::now();

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
	usleep(daq::kShortSleep);
      }
    }

    std::this_thread::yield();
    usleep(daq::kLongSleep);
  }
}

caen_1785 DaqWorkerCaen1785::PopEvent()
{
  static caen_1785 data;
  queue_mutex_.lock();

  if (data_queue_.empty()) {

    caen_1785 str;
    queue_mutex_.unlock();
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


bool DaqWorkerCaen1785::EventAvailable()
{
  // Check acq reg.
  static ushort msg_ushort= 0;
  static uint msg;
  static bool is_event;

  // Check if the device has data.
  Read16(0x100E, msg_ushort);
  is_event = (msg_ushort & 0x1);
  
  // Check to make sure the buffer isn't empty.
  Read16(0x1022, msg_ushort);
  is_event &= !(msg & 0x2);

  return is_event;
}

void DaqWorkerCaen1785::GetEvent(caen_1785 &bundle)
{
  int offset = 0x0;
  uint ch = 0;
  uint data = 0;

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();

  // Read the data for each high value.
  while ((((data >> 24) & 0x7) != 0x6) || (((data >> 24) &0x7) != 0x4)) {

    if (offset >= 0x1000) break;

    Read(offset, data);
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
	Write16(0x1028, 0x0);
	break;
      }

      // Device in order 0, 4, 1, 5, 2, 6, 3, 7.
      ch = (ch > 3) * (ch - 3) + (ch < 4) * (ch + 4);
    }
  }
}

} // ::daq
