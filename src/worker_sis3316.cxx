#include "worker_sis3316.hh"

namespace daq {

WorkerSis3316::WorkerSis3316(std::string name, std::string conf) : 
  WorkerVme<sis_3316>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3316_CH;
  read_trace_len_ = 3 + SIS_3316_LN / 2; // only for vme ReadTrace
  read_trace_len_ += (read_trace_len_ % 2); // needs to be even
  bank2_armed_flag = false;
}

void WorkerSis3316::LoadConfig()
{ 
  using std::string;

  int rc = 0, gr = 0;
  uint msg = 0, addr = 0;
  
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);
  
  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoul(conf.get<string>("base_address"), nullptr, 0);
  
  // Read the base register.
  rc = Read(CONTROL_STATUS, msg);
  if (rc == 0) {

    LogMessage("SIS3316 found at 0x%08x", base_address_);
    
  } else {
    
    LogError("SIS3316 at 0x%08x could not be found", base_address_);
  }

  // Get device ID.
  rc = Read(MODID, msg);
  if (rc != 0) {

    LogError("failed to read device ID register");

  } else {

    LogMessage("ID: %04x, maj rev: %02x, min rev: %02x",
	       msg >> 16, (msg >> 8) & 0xff, msg & 0xff);
  }
  
  // Get device hardware revision.
  rc = Read(HARDWARE_VERSION, msg);
  if (rc == 0) {
    
    LogMessage("device hardware version %i", msg & 0xf);
    
  } else {

    LogError("failed to read device hardware revision");
  }

  // Get ADC fpga firmware revision.
  
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    addr = CH1_4_FIRMWARE + kAdcRegOffset * gr;

    rc = Read(addr, msg);
  
    if (rc != 0) {
      LogError("failed to read device hardware revision");
    }

    LogMessage("ADC%i fpga firmware version 0x%08x", gr, msg);
  }
  
  // Check the board temperature
  rc = Read(INTERNAL_TEMPERATURE, msg);
  if (rc == 0) {
    
    short temp = (short)msg &0xffff;
    LogMessage("device internal temperature is %0.2fC", temp * 0.25);
    
  } else {
    
    LogError("failed to read device temperature");
  }
  
  // Reset the device.
  rc = Write(KEY_RESET, 0x1);
  if (rc != 0) {
    LogError("failure to reset device");
  }

  // Disarm the device.
  rc = Write(KEY_DISARM, 0x1);
  if (rc != 0) {
    LogError("failure to disarm device");
  }
  
  // SPI setup here. First disable ADC chip outputs.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set address to ADC's SPI_CTRL_REG.
    addr = CH1_4_SPI_CTRL + kAdcRegOffset * gr;
    rc = Write(addr, 0x0);

    if (rc != 0) {
      LogError("failure disabling ADC output");
    }
  }

  // Soft reset the ADC.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    rc = AdcSpiWrite(gr, 0, 0x0, 0x24);
    rc &= AdcSpiWrite(gr, 1, 0x0, 0x24);
    
    if (rc != 0) {
      LogError("failure to reset ADCs");
    }
  }

  // Set reference voltage.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    rc = AdcSpiWrite(gr, 0, 0x14, 0x40);
    rc &= AdcSpiWrite(gr, 1, 0x14, 0x40);

    if (rc != 0) {
      LogError("failure to set ADC reference voltage (2.0V)");
    }
  }

  // Select output mode.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    rc = AdcSpiWrite(gr, 0, 0x18, 0xc0);
    rc &= AdcSpiWrite(gr, 1, 0x18, 0xc0);

    if (rc != 0) {
      LogError("failure to select ADC output mode");
    }
  }

  // Register update.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    rc = AdcSpiWrite(gr, 0, 0xff, 0x1);
    rc &= AdcSpiWrite(gr, 1, 0xff, 0x1);

    if (rc != 0) {
      LogError("failure to update ADCs");
    }
  }

  // Enable output again.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set address to ADC's SPI_CTRL_REG.
    addr = CH1_4_SPI_CTRL + kAdcRegOffset * gr;
    rc = Write(addr, 0x01000000);

    if (rc != 0) {
      LogError("failure enabling ADC output");
    }
  }

  // Enable external LEMO trigger.
  msg = 0;
  if (conf.get<bool>("enable_ext_trg", true)) {
    msg |= (0x1 << 4); // enable external trigger bit
  }
  
  if (conf.get<bool>("invert_ext_trg", false)) {
    msg |= (0x1 << 5); // invert external trigger bit
  }

  if (conf.get<bool>("enable_ext_clk", false)) {
    msg |= (0x1 << 0); // enable external clock
  }  

  // Write to NIM_INPUT_CTRL
  rc = Write(NIM_INPUT_CONTROL, msg);

  if (rc != 0) {
    LogError("failure to write NIM input control register");
  }

  if (conf.get<bool>("enable_ext_clk", false)) {

    // // Reset the device again.
    // rc = Write(KEY_RESET, 0x1);
    // if (rc != 0) {
    //   LogError("failure to reset device");
    // }
    
    // Disarm the device.
    rc = Write(KEY_DISARM, 0x1);
    if (rc != 0) {
      LogError("failure to disarm device");
    }
    
    // Set the ADC clock distribution.
    if (Write(SAMPLE_CLOCK_DISTRIBUTION_CONTROL, 0x3) != 0) {
      LogError("failure to set ADC clock mux");      
    }

    // Enable external LEMO trigger.
    msg = 0;
    if (conf.get<bool>("enable_ext_trg", true)) {
      msg |= (0x1 << 4); // enable external trigger bit
    }
  
    if (conf.get<bool>("invert_ext_trg", false)) {
      msg |= (0x1 << 5); // invert external trigger bit
    }

    if (conf.get<bool>("enable_ext_clk", false)) {
      msg |= (0x1 << 0); // enable external clock
    }  

    // Write to NIM_INPUT_CTRL
    rc = Write(NIM_INPUT_CONTROL, msg);

    if (rc != 0) {
      LogError("failure to write NIM input control register");
    }

    // Reset the Si5325.
    if (Si5325Write(0x88, 0x80) != 0) {
      LogError("failure to reset the Si5325");
    }
    usleep(12000);

    // Program the clock multiplier
    if (Si5325Write(0x0, 0x2) != 0) {
      LogError("failure to select address for clock multipler spi register");
    }

    // Power down clk2.
    if (Si5325Write(0xb, 0x2) != 0) {
      LogError("failure to power down clk2");
    }

    // Trigger an internal calibration
    if (Si5325Write(0x88, 0x40) != 0) {
      LogError("failure to trigger internal calibration on Si5325");
    }

    usleep(25000);

  } else {

    // // Reset the device again.
    // rc = Write(KEY_RESET, 0x1);
    // if (rc != 0) {
    //   LogError("failure to reset device");
    // }
    
    // Disarm the device.
    rc = Write(KEY_DISARM, 0x1);
    if (rc != 0) {
      LogError("failure to disarm device");
    }

    SetOscFreqHSN1(conf.get<int>("oscillator_num", 0),
                   conf.get<unsigned char>("oscillator_hs", 5),
                   conf.get<unsigned char>("oscillator_n1", 8));
  }

  // Issue a DCM/PLL reset.
  if (Write(KEY_ADC_CLOCK_DCM_RESET, msg) != 0) {
    LogError("failure to reset DCM/PLL");
  }

  // Give it a bit to finish.
  usleep(5000);

  // Enable ADC chip outputs.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set address to ADC's SPI_CTRL_REG.
    addr = CH1_4_SPI_CTRL + kAdcRegOffset * gr;
    rc = Write(addr, 0x01000000);

    if (rc != 0) {
      LogError("failure enabling ADC output");
    }
  }

  // Calibrate IOB delay logic.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {

    // Set address to ADC's INPUT_TAP_DELAY_REG.
    addr = CH1_4_INPUT_TAP_DELAY + kAdcRegOffset * gr;

    // Calibrate all channels.
    rc = Write(addr, 0xf00); 

    if (rc != 0) {
      LogError("failure calibrating IOB tap delay logic");
    }
  }
  usleep(100);

  for (gr = 0; gr < SIS_3316_GR; ++gr) {

    // Set address to ADC's INPUT_TAP_DELAY_REG
    addr = CH1_4_INPUT_TAP_DELAY + kAdcRegOffset * gr;
    string hex_tap_delay = conf.get<string>("iob_tap_delay", "0x1020");
    uint iob_tap_delay = std::stoi(hex_tap_delay, nullptr, 0);

    if (iob_tap_delay & 0xffff0000 != 0) {
      iob_tap_delay &= 0xffff;

      LogWarning("iob_tap_delay truncated to 2 bytes");
    }
    
    // Set value specific to fpga and clock frequency.
    // A few examples (see more on manual page 116):
    // 125 MHz: fpga_0003 = 0x7f 
    // 125 MHz: fpga_0004 = 0x1020 
    // 250 MHz: fpga_0003 = 0x48 
    // 250 MHz: fpga_0004 = 0x1008
    // (+ 0x300) is to select all channels.
    rc = Write(addr, 0x300 + iob_tap_delay);

    if (rc != 0) {
      LogError("failure setting IOB tap delay logic");
    }
  }
  usleep(100);
    
  // Write to the channel header registers.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {

    addr = CH1_4_CHANNEL_HEADER + kAdcRegOffset * gr;
    msg = 0x400000 * gr;
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure to write channel header register");
    }
  }

  // Set the DAC offsets by groups of 4 channels
  if (conf.get<bool>("set_voltage_offset", true)) {

    for (gr = 0; gr < SIS_3316_GR; ++gr) {
      
      addr = CH1_4_DAC_OFFSET_CTRL + kAdcRegOffset * gr;
      
      // Enable the internal reference.
      rc = Write(addr, 0x88f00001); // Magic constant found in Struck source.

      if (rc != 0) {
	LogError("failed to enable the internal reference for ADC %i", gr + 1);
      }
      usleep(1000);  // update takes time

      string dac_offset_hex = conf.get<string>("dac_voltage_offset", "0x8000");
      uint offset = std::stoi(dac_offset_hex, nullptr, 0);
      
      if ((offset & 0xffff0000) != 0) {
        offset &= 0xffff;
        
        LogWarning("truncating DAC offset value to 2 bytes");
      }

      // Set the voltage offset and write it for all channels.
      msg = 0;
      msg |= (0x8 << 28);
      msg |= (0x2 << 24);
      msg |= (0xf << 20);
      msg |= (offset << 4);

      rc = Write(addr, msg);
      if (rc != 0) {
	LogError("failure to write offset for DAC %i", gr + 1);
      }

      // Tell the DAC to load the values.
      rc = Write(addr, 0x3 << 30);
      if (rc != 0) {
	LogError("failure to load offset for DAC %i", gr + 1);
      }
      usleep(1000);
    }
  }

  // Check the DAC offset readback registers.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {
    
    addr = CH1_4_DAC_OFFSET_READBACK + kAdcRegOffset * gr;

    rc = Read(addr, msg);
    if (rc != 0) {

      LogError("failure checking DAC offset readback register");

    } else {

      LogMessage("DAC offset readback register is 0x%08x", msg);
    }
  }

  // Set the trigger gate window and raw data buffer length.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {

    // First the trigger gate length, doesn't effect output trace length.
    addr = CH1_4_TRIGGER_GATE_WINDOW_LENGTH + kAdcRegOffset * gr;
    msg = (SIS_3316_LN - 2) & 0xffff;

    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure to set the trigger gate window");
    }

    // Now the number of samples per trace.
    addr = 0x1020 + kAdcRegOffset * gr;
    msg = (SIS_3316_LN << 16) | (0 & 0xffff); // 0 is start address in ADC
      
    rc = Write(addr, msg);
    if (rc != 0) {
      LogError("failure to set the raw data buffer length");
    }

    // Write to the extended length register if the trace is too long.
    if (SIS_3316_LN > 0xffff) {
      addr = CH1_4_EXTENDED_RAW_DATA_BUFFER_CONFIG + kAdcRegOffset * gr;

      if ((SIS_3316_LN & 0xfe000000) != 0) {
        LogWarning("event length truncated to maximum value, 0x1ffffff");
      }

      msg = SIS_3316_LN & 0x1ffffff; // 25 bits total.
      rc = Write(addr, msg);
      
      if (rc != 0) {
        LogError("failure to set the raw data buffer length");
      }
    }
  }

  // Set the pre-trigger buffer length for each channel.
  msg = std::stoi(conf.get<string>("pretrigger_samples", "0x0"), nullptr, 0);
  
  if ((msg & 0xffffc) != 0) {
    LogWarning("pre-trigger delay truncated to max of 0x1ffe");
  }

  msg &= 0x3fff; // max of 2042 and bit 0 = 0

  for (gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set the to address of ADC's pre-trigger configuration.
    addr = CH1_4_PRE_TRIGGER_DELAY + kAdcRegOffset * gr;
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure setting pre-trigger for channel group %i", gr + 1);
    }
  }

  // Need to enable triggers per channel also, I think.
  for (gr = 0; gr < SIS_3316_GR; ++gr) {
    
    addr = CH1_4_EVENT_CONFIG + kAdcRegOffset * gr;

    rc = Read(addr, msg);
    if (rc != 0) {
      LogError("failure reading event config for channel group %i", gr + 1);
    }
    
    if (conf.get<bool>("enable_ext_trg", true)) {
      msg |= 0x1 << 3;
      msg |= 0x1 << 11;
      msg |= 0x1 << 19;
      msg |= 0x1 << 27;
    }

    if (conf.get<bool>("enable_int_trg", false)) {
      msg |= 0x1 << 2;
      msg |= 0x1 << 10;
      msg |= 0x1 << 18;
      msg |= 0x1 << 26;
    }

    rc = Write(addr, msg);
    if (rc != 0) {
	LogError("failure writing event config for channel group %i", gr + 1);
    }
  }

  // Set the data format and address thresholds.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Data format
    addr = CH1_4_DATAFORMAT_CONFIG + kAdcRegOffset * gr;
    rc = Write(addr, 0x0);

    if (rc != 0) {
      LogError("failed to set data format for ADC %i", gr);
    }

    // Address threshold
    addr = CH1_4_ADDRESS_THRESHOLD + kAdcRegOffset * gr;
    read_trace_len_ = 1 * (3 + SIS_3316_LN / 2);
    rc = Write(addr, read_trace_len_ - 1);

    if (rc != 0) {
      LogError("failed to set address threshold for ADC %i", gr);
    }
  }

  // Enable external timestamp clear.
  msg |= 0x10; 
  rc = Write(ACQUISITION_CONTROL, msg);

  if (rc != 0) {
    LogError("failure to write acquisition control register");
  }

  // Enable clock output
  rc = Write(LEMO_OUT_CO_SELECT, 0x1);
  if (rc != 0) {
    LogError("failure to enable CO clock out");
  }

  // Trigger a timestamp clear.
  rc = Write(KEY_TIMESTAMP_CLR, 0x1);
  if (rc != 0) {
    LogError("failed to clear timestamp");
  }

  // Arm bank1 to start.
  rc = Write(KEY_DISARM_AND_ARM_BANK1, 0x1);
  
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
  static bool is_event;
  static int count, rc;
  static uint msg;

  count = 0;
  msg = 0;
  do {
    rc = Read(ACQUISITION_CONTROL, msg);
  } while ((rc != 0) && (count++ < kMaxPoll));
 
  // Check memory threshold flag
  is_event = msg & (0x1 << 19);

  // Switch banks and rearm the logic.
  if (is_event && go_time_) {

    count = 0;
    rc = 0;

    if (bank2_armed_flag) {

      do {
	rc = Write(KEY_DISARM_AND_ARM_BANK1, 1);
	bank2_armed_flag = false;
    
      } while ((rc != 0) && (count++ < kMaxPoll));

    } else {

      do {
	rc = Write(KEY_DISARM_AND_ARM_BANK2, 1);
	bank2_armed_flag = true;
    
      } while ((rc != 0) && (count++ < kMaxPoll));

      if (rc != 0) {
        LogError("failed to write to trigger logic register");
      }

      if (count >= kMaxPoll) {
        LogError("timed out attempting to set trigger logic register");
      }
    }

    return is_event;
  }

  return false;
}

void WorkerSis3316::GetEvent(sis_3316 &bundle)
{
  using namespace std::chrono;
  int ch, rc, count = 0;
  uint trace_addr, addr, offset, msg;

  // Check how long the event is.
  uint next_sample_address[SIS_3316_CH];
  static uint data[SIS_3316_CH][3 + SIS_3316_LN / 2];

  // Get the system time.
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  
  LogMessage("GetEvent start: %ul", duration_cast<microseconds>(dtn).count());

  // Now get the raw data (timestamp and waveform).
  for (ch = 0; ch < SIS_3316_CH; ch++) {

    // Calculate the register for previous address.
    offset = CH1_PREVIOUS_SAMPLE_ADDRESS + kAdcRegOffset * (ch >> 2);
    offset += 0x4 * (ch % SIS_3316_GR);

    // Read out the previous address.
    count = 0;
    do {

      rc = Read(offset, msg);

      if (rc != 0) {
	LogError("failure reading address for channel %i", ch);
      }

      if (count++ > kMaxPoll) {
	LogError("read event timed out");
	return;
      }

    } while ((msg & 0x01000000) != (!bank2_armed_flag << 24));

    if ((msg & 0xffffff) == 0) {
      LogError("no data received");
      return;
    }
    
    // Specify the channel's fifo address
    msg = 0x80000000; // Start transfer bit
    if (!bank2_armed_flag) msg += 0x01000000; // Bank 2 offset
    if ((ch & 0x1) == 0x1) msg += 0x02000000; // ch 2, 4, 6, ...
    if ((ch & 0x2) == 0x2) msg += 0x10000000; // ch 2, 3, 6, 7, ...

    // Start readout FSM
    addr = DATA_TRANSFER_ADC1_4_CTRL + 0x4 * (ch >> 2);
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failed begin data tranfer for channel %i", ch);
    }
    usleep(5);

    // Set to the adc base memory.
    count = 0;
    trace_addr = 0x100000 * ((ch >> 2) + 1);
    LogMessage("attempting to read trace at 0x%08x", trace_addr);

    do {
      rc = ReadTraceMblt64Fifo(trace_addr, data[ch]);

      if (rc != 0) {
        LogError("failed to read trace for channel %i", ch);
      }
    } while ((rc != 0) && (count++ < kMaxPoll));


    if (count >= kMaxPoll) {
      LogError("timed out reading trace for channel %i", ch);
    }

    // Reset the FSM
    rc = Write(addr, 0x0);
    if (rc != 0) {
      LogError("failed reset data tranfer for channel %i", ch);
    }
  }

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3316_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = data[ch][1] & 0xffff;
    bundle.device_clock[ch] |= data[ch][1] & (0xffff << 16);
    bundle.device_clock[ch] |= (data[ch][0] & 0xffffULL << 16) << 32;

    std::copy((ushort *)(data[ch]+3),
	      (ushort *)(data[ch]+3) + SIS_3316_LN,
    	      bundle.trace[ch]);
  }

  t1 = high_resolution_clock::now();
  dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  LogMessage("GetEvent stop: %ul", duration_cast<microseconds>(dtn).count());
}

int WorkerSis3316::I2cStart(int osc)
{
  if (osc < 0 || osc > 3) {
    LogError("I2C oscillator out of range, 0-3");
    return 1;
  }

  uint addr = 0, msg = 0, count = 0;

  // Set address to specified oscillator register (0 for int, 1|2 for ext).
  addr = ADC_CLK_OSC_I2C + 0x4 * osc;

  // Write to START bit.
  if (Write(addr, 0x1 << 9) != 0) {
    LogError("failed to start I2C oscillator %i", osc);
  }

  // Read until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
    LogMessage("I2cStart response: 0x%08x", msg);
  } while ((count++ < kMaxPoll) && ((msg >> 31) & 0x1));

  if (count >= kMaxPoll) return 2;

  return 0;
}

int WorkerSis3316::I2cStop(int osc)
{
  if (osc < 0 || osc > 3) {
    LogError("I2C oscillator out of range, 0-3");
    return 1;
  }

  uint addr = 0, msg = 0, count = 0;
  
  // Set address to specified oscillator register (0 for int, 1|2 for ext).
  addr = ADC_CLK_OSC_I2C + 0x4 * osc;
  
  // Write to STOP bit.
  if (Write(addr, 0x1 << 11) != 0) {
    LogError("failed to stop I2C oscillator 0");
  }

  // Read until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
    LogMessage("I2cStop response: 0x%08x", msg);
  } while ((count++ < kMaxPoll) && ((msg >> 31) & 0x1));

  
  if (count >= kMaxPoll) {
    LogError("I2C busy, timed out");
    return 2;
  }

  return 0;
}

int WorkerSis3316::I2cRead(int osc, unsigned char &data, unsigned char ack)
{
  if (osc < 0 || osc > 3) {
    LogError("I2C oscillator out of range, 0-3");
    return 1;
  }

  uint addr = 0, msg = 0, count = 0;
  
  // Set address to specified oscillator register (0 for int, 1|2 for ext).
  addr = ADC_CLK_OSC_I2C + 0x4 * osc;

  // Set the message READ bit and ACK bit.
  msg = (0x1 << 13);
  msg |= ack ? (1 << 8) : 0;
  
  if (Write(addr, msg) != 0) {
    LogError("failed to request READ from I2C address");
  }

  // Attempt reads until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
  } while ((count++ < kMaxPoll) && ((msg >> 31) & 0x1));

  if (count >= kMaxPoll) {
    LogError("I2C busy timed out");
    return 2;
  }

  // Data is only lowest byte.
  data = (unsigned char) (msg & 0xff);

  return 0;
}

int WorkerSis3316::I2cWrite(int osc, unsigned char data, unsigned char &ack)
{
  if (osc < 0 || osc > 3) {
    LogError("I2C oscillator out of range, 0-3");
    return 1;
  }

  uint addr = 0, msg = 0, count = 0;

  // Set address to specified oscillator register (0 for int, 1|2 for ext).
  addr = ADC_CLK_OSC_I2C + 0x4 * osc;

  // Set the message data and WRITE bit.
  msg = (0x1 << 12) ^ data;

  if (Write(addr, msg) != 0) {
    LogError("failed to request WRITE to I2C address");
  }

  // Read until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
    LogMessage("I2cWrite response: 0x%08x", msg);
  } while ((count++ < kMaxPoll) && ((msg >> 31) & 0x1));

  if (count >= kMaxPoll) {
    LogError("I2C busy timed out");
    return 2;
  }
  
  // Check the ACK bit.
  ack = ((msg >> 8) & 0x1) ? 1 : 0;

  return 0;
}

int WorkerSis3316::SetOscFreqHSN1(int osc, unsigned char hs, unsigned char n1)
{
  // Set the sample clock
  int rc;
  unsigned char ack;
  unsigned char freq_high_speed_rd[6];
  unsigned char freq_high_speed_wr[6];

  // Start.
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failed to start I2C");
    rc = I2cStop(osc);
  }

  // Set address.
  rc = I2cWrite(osc, (0x55 << 1), ack);
  if (rc != 0) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  } else if (!ack) {
    LogError("failure to receive ack after writing I2C byte");
    I2cStop(osc);
  }    

  // Register offset.
  rc = I2cWrite(osc, 0x0d, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }

  // Test start.
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failed to start I2C");
    rc = I2cStop(osc);
  }

  // Set address + 1.
  rc = I2cWrite(osc, (0x55 << 1) + 1, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }

  // Now readout the 6 bytes currently in there.
  for (int i = 0; i < 6; ++i) {
    
    if (i == 5) {
      ack = 0;
    } else {
      ack = 1;
    }

    rc = I2cRead(osc, freq_high_speed_rd[i], ack);
    if (rc != 0) {
      LogError("failed to write READ/ACK to I2C 0 register");
      I2cStop(osc);
    }
  }

  rc = I2cStop(osc);
  if (rc != 0) {
    LogError("failed stopping I2C command");
  }

  // Now set the clock write bits as done in STRUCK source code.
  freq_high_speed_wr[0] = ((hs & 0x7) << 5) + ((n1 & 0x7c) >> 2);
  freq_high_speed_wr[1] = ((n1 & 0x3) << 6) + freq_high_speed_rd[1] & 0x3f;
  freq_high_speed_wr[2] = freq_high_speed_rd[2];
  freq_high_speed_wr[3] = freq_high_speed_rd[3];
  freq_high_speed_wr[4] = freq_high_speed_rd[4];
  freq_high_speed_wr[5] = freq_high_speed_rd[5];

  // Si570FreezeDCO
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failure starting I2C command");
    rc = I2cStop(osc);
  }

  // Set Address.
  rc = I2cWrite(osc, 0x55 << 1, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C address byte");
    I2cStop(osc);
  }
  
  // Set Offset.
  rc = I2cWrite(osc, 0x89, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C offset byte");
    I2cStop(osc);
  }

  // Write data.
  rc = I2cWrite(osc, 0x10, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C data byte");
    I2cStop(osc);
  }  

  // And stop command.
  rc = I2cStop(osc);

  // Si570Divider
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failure starting I2C command");
    rc = I2cStop(osc);
  }

  // Set Address.
  rc = I2cWrite(osc, 0x55 << 1, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }
  
  // Set Offset.
  rc = I2cWrite(osc, 0x0d, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }

  // Write data.
  for (int i = 0; i < 6; ++i) {
    rc = I2cWrite(osc, freq_high_speed_wr[i], ack);
    if ((rc != 0) || (!ack)) {
      LogError("failure writing I2C byte");
      I2cStop(osc);
    }  
  }

  // And stop command.
  rc = I2cStop(osc);

  // Si570UnfreezeDCO
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failure starting I2C command");
    rc = I2cStop(osc);
  }

  // Set Address.
  rc = I2cWrite(osc, 0x55 << 1, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }
  
  // Set Offset.
  rc = I2cWrite(osc, 0x89, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }

  // Write data.
  rc = I2cWrite(osc, 0x00, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }  

  // And stop command.
  rc = I2cStop(osc);

  // si570NewFreq
  rc = I2cStart(osc);
  if (rc != 0) {
    LogError("failure starting I2C command");
    rc = I2cStop(osc);
  }

  // Set Address.
  rc = I2cWrite(osc, 0x55 << 1, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }
  
  // Set Offset.
  rc = I2cWrite(osc, 0x87, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }

  // Write data.
  rc = I2cWrite(osc, ADC_CLK_OSC_I2C, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }  

  // And stop command.
  rc = I2cStop(osc);
  
  // Wait, then reset the DCM.
  usleep(15000);
  rc = Write(KEY_ADC_CLOCK_DCM_RESET, 0x0);
  if (rc != 0) {
    LogError("failed to reset the DCM");
  }
  // And wait for reset.
  usleep(5000);

return 0;
}

int WorkerSis3316::AdcSpiWrite(int gr, int chip, uint spi_addr, uint msg)
{
  int rc;
  uint data = 0, adc_mux = 0, addr = 0, count = 0, maxcount = 1000;

  if ((gr > 4) || ((chip > 2) || (spi_addr > 0xffff))) {
    return -1;
  }

  if (chip == 0) {
    adc_mux = 0;
  } else {
    adc_mux = 0x1 << 22;
  }

  // Set to appropriate SPI_CTRL register.
  addr = CH1_4_SPI_CTRL + gr * kAdcRegOffset;

  if (Read(addr, data) != 0) {
    return -1;
  }
  
  // Preserve the enable bit and add command bit and mux bit.
  data &= (0x1 << 24);
  data += 0x80000000 + adc_mux + (spi_addr << 8) + (msg & 0xff);

  if (Write(addr, data) != 0) {
    return -1;
  }

  // Check busy register, bit 31 to make sure it finishes before returning.
  addr = 0xa4;
  
  do {
    rc = Read(addr, data);
  } while (((rc != 0) && (count++ < maxcount)) && (data & (0x1 << 31)));
  
  if (count >= maxcount) {
    return -2;
  }
  
  return rc;
}

int WorkerSis3316::Si5325Write(uint addr, uint msg)
{
  uint count = 0, maxpoll = 100, tmp = 0;

  // Select the address for writing.
  if (Write(NIM_CLK_MULTIPLIER_SPI, addr & 0xff) != 0) {
    LogError("failure to select address for clock multipler spi register");
  }
  
  // Wait for the SPI to finish.
  do {
    if (Read(NIM_CLK_MULTIPLIER_SPI, tmp) != 0) {
      LogError("SI5235 SPI write failed");
      return -1;
    } 
  } while (((0x80000000 & tmp) == 0x80000000) && (count++ < maxpoll));

  if (count >= maxpoll) {
    LogError("SI5235 SPI busy polling timed out");
    return -2;
  }

  // Write the data (0xff) and instructions (0xff00).
  if (Write(NIM_CLK_MULTIPLIER_SPI, 0x4000 + (msg & 0xff)) != 0) {
    LogError("SI5235 SPI write failed");
    return -1;
  }

  // Wait for the SPI command to finish again.
  count = 0;
  do {
    if (Read(NIM_CLK_MULTIPLIER_SPI, tmp) != 0) {
      LogError("SI5235 SPI busy poll failed");
      return -1;
    }
  } while (((0x80000000 & tmp) == 0x80000000) && (count++ < maxpoll));

  if (count >= maxpoll) {
    LogError("SI5235 SPI busy polling timed out");
    return -2;
  }

  return 0;
}

} // ::daq
