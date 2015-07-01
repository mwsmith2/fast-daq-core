#include "worker_sis3316.hh"

namespace daq {

WorkerSis3316::WorkerSis3316(std::string name, std::string conf) : 
  WorkerVme<sis_3316>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3316_CH;
  read_trace_len_ = 3 + SIS_3316_LN / 2; // only for vme ReadTrace
  bank2_armed_flag = false;
}

void WorkerSis3316::LoadConfig()
{ 
  using std::string;

  int rc = 0;
  uint msg = 0;
  uint addr = 0;
  
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);
  
  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoul(conf.get<string>("base_address"), nullptr, 0);
  
  // Read the base register.
  rc = Read(0x0, msg);
  if (rc == 0) {

    LogMessage("SIS3316 found at 0x%08x", base_address_);
    
  } else {
    
    LogError("SIS3316 at 0x%08x could not be found", base_address_);
  }
    
  // Get device ID.
  rc = Read(0x4, msg);
  
  if (rc != 0) {

    LogError("failed to read device ID register");

  } else {

    LogMessage("ID: %04x, maj rev: %02x, min rev: %02x",
	       msg >> 16, (msg >> 8) & 0xff, msg & 0xff);
  }
  
  // Get device hardware revision.
  rc = Read(0x1c, msg);
  
  if (rc == 0) {
    
    LogMessage("device hardware version %i", msg & 0xf);
    
  } else {

    LogError("failed to read device hardware revision");
  }

  // Get ADC fpga firmware revision.
  
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    addr = 0x1100 + kAdcRegOffset * gr;

    rc = Read(addr, msg);
  
    if (rc != 0) {
      LogError("failed to read device hardware revision");
    }

    LogMessage("ADC%i fpga firmware version 0x%08x", gr, msg);
  }
  
  // Check the board temperature
  rc = Read(0x20, msg);
  
  if (rc == 0) {
    
    LogMessage("device internal temperature is %0.2fC", (ushort)(msg) * 0.25);
    
  } else {
    
    LogError("failed to read device temperature");
  }
  
  // Reset the device.
  rc = Write(0x400, 0x1);
  if (rc != 0) {
    LogError("failure to reset device");
  }

  // Disarm the device.
  rc = Write(0x414, 0x1);
  if (rc != 0) {
    LogError("failure to disarm device");
  }
  
  // SPI setup here. First disable ADC chip outputs.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set address to ADC's SPI_CTRL_REG.
    addr = 0x100c + kAdcRegOffset * gr;
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
    addr = 0x100c + kAdcRegOffset * gr;
    rc = Write(addr, 0x01000000);

    if (rc != 0) {
      LogError("failure enabling ADC output");
    }
  }

  // Reset the device again.
  rc = Write(0x400, 0x1);
  if (rc != 0) {
    LogError("failure to reset device");
  }

  // Disarm the device.
  rc = Write(0x414, 0x1);
  if (rc != 0) {
    LogError("failure to disarm device");
  }

  SetOscFreqHSN1(conf.get<int>("oscillator_num", 0),
     		 conf.get<unsigned char>("oscillator_hs", 5),
     		 conf.get<unsigned char>("oscillator_n1", 8));

  // Enable ADC chip outputs.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set address to ADC's SPI_CTRL_REG.
    addr = 0x100c + kAdcRegOffset * gr;
    rc = Write(addr, 0x01000000);

    if (rc != 0) {
      LogError("failure enabling ADC output");
    }
  }

  // Calibrate IOB delay logic.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    // Set address to ADC's INPUT_TAP_DELAY_REG.
    addr = 0x1000 + kAdcRegOffset * gr;

    // Calibrate all channels.
    rc = Write(addr, 0xf00); 

    if (rc != 0) {
      LogError("failure calibrating IOB tap delay logic");
    }
  }
  usleep(100);

  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    // Set address to ADC's INPUT_TAP_DELAY_REG
    addr = 0x1000 + kAdcRegOffset * gr;
    string hex_tap_delay = conf.get<string>("iob_tap_delay", "0x1020");
    int iob_tap_delay = std::stoi(hex_tap_delay, nullptr, 0);
    
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
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    addr = 0x1014 + kAdcRegOffset * gr;
    msg = 0x400000 * gr;
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure to write channel header register");
    }
  }

  // Set the DAC offsets by groups of 4 channels
  if (conf.get<bool>("set_voltage_offsets", true)) {
    
    for (int gr = 0; gr < SIS_3316_GR; ++gr) {
      
      addr = 0x1008 + kAdcRegOffset * gr;
      
      // Enable the internal reference.
      rc = Write(addr, 0x88f00001); // Magic constant found in Struck source.
      if (rc != 0) {
	LogError("failed to enable the internal reference for ADC %i", gr + 1);
      }
      usleep(1000);  // update takes time

      // Set the voltage offset and write it for all channels.
      msg = 0;
      msg |= (0x8 << 28);
      msg |= (0x2 << 24);
      msg |= (0xf << 20);
      msg |= (0x8000 << 4);

      LogWarning("attempting to set voltage offset to default of 0x8000");

      rc = Write(addr, msg);
      if (rc != 0) {
	LogError("failure to write offset for DAC %i", gr+1);
      }

      // Tell the DAC to load the values.
      msg = 0;
      msg |= 0xc0000000;

      rc = Write(addr, msg);
      if (rc != 0) {
	LogError("failure to load offset for DAC %i", gr+1);
      }
      usleep(1000);
    }
  }

  // Check the DAC offset readback registers.
  for (uint gr = 0; gr < SIS_3316_GR; ++gr) {
    
    addr = 0x1108 + kAdcRegOffset * gr;
    rc = Read(addr, msg);

    if (rc != 0) {

      LogError("failure checking DAC offset readback register");

    } else {

      LogMessage("DAC offset readback register is 0x%08x", msg);
    }
  }

  // Set the trigger gate window and raw data buffer length.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {

    // First the trigger gate length
    addr = 0x101c + kAdcRegOffset * gr;
    msg = (SIS_3316_LN - 2) & 0xffff;

    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure to set the trigger gate window");
    }

    // Now the number of raw data samples
    addr = 0x1020 + kAdcRegOffset * gr;
    msg = (SIS_3316_LN << 16) | (0 & 0xffff); // 0 is start address in ADC
    
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure to set the raw data buffer length");
    }
  }

  // Set the pre-trigger buffer length for each channel.
  msg = std::stoi(conf.get<string>("pretrigger_samples", "0x0"), nullptr, 0);
  msg &= 0x1ffe; // max of 2042 and bit 0 = 0

  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Set the to address of ADC's pre-trigger configuration.
    addr = 0x1028 + kAdcRegOffset * gr;

    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failure setting pre-trigger for channel group %i", gr + 1);
    }
  }

  // Enable external LEMO trigger.
  msg = 0;
  if (conf.get<bool>("enable_ext_lemo", true)) {
    msg |= (0x1 << 4); // enable external trigger bit
  }
  
  if (conf.get<bool>("invert_ext_lemo", false)) {
    msg |= (0x1 << 5); // invert external trigger bit
  }
  
  // Write to NIM_INPUT_CTRL
  rc = Write(0x5c, msg);

  if (rc != 0) {
    LogError("failure to write NIM input control register");
  }

  // Need to enable triggers per channel also, I think.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    uint reg = 0x1010 + kAdcRegOffset * gr;

    rc = Read(reg, msg);
    if (rc != 0) {
      LogError("failure reading event config for channel group %i", gr + 1);
    }
    
    if (conf.get<bool>("enable_ext_trigger", true)) {
      msg |= 0x1 << 3;
      msg |= 0x1 << 11;
      msg |= 0x1 << 19;
      msg |= 0x1 << 27;
    }

    if (conf.get<bool>("enable_int_trigger", false)) {
      msg |= 0x1 << 2;
      msg |= 0x1 << 10;
      msg |= 0x1 << 18;
      msg |= 0x1 << 26;
    }

    rc = Write(reg, msg);
    if (rc != 0) {
	LogError("failure writing event config for channel group %i", gr + 1);
    }
  }

  // Set the data format and address thresholds.
  for (int gr = 0; gr < SIS_3316_GR; ++gr) {
    
    // Data format
    addr = 0x1030 + kAdcRegOffset * gr;
    
    rc = Write(addr, 0x0);
    
    if (rc != 0) {
      LogError("failed to set data format for ADC %i", gr);
    }

    // Address threshold
    addr = 0x1018 + kAdcRegOffset * gr;
    read_trace_len_ = 1 * (3 + SIS_3316_LN / 2);
    rc = Write(addr, read_trace_len_ - 1);

    if (rc != 0) {
      LogError("failed to set address threshold for ADC %i", gr);
    }
  }

  // Enable trigger on acqusition control
  msg = 0;
  if (conf.get<bool>("enable_ext_trigger", true)) {
    //   msg |= 0x1 << 15; // external trigger disable with internal busy
     msg |= 0x1 << 8;
  }

  if (conf.get<bool>("enable_int_trigger", false)) {
     msg |= 0x1 << 14;
  }

  msg |= 0x400; // Enable external timestamp clear.

  rc = Write(0x60, msg);
  if (rc != 0) {
    LogError("failure to write acquisition control register");
  }

  // Trigger a timestamp clear.
  rc = Write(0x41c, 0x1);
  if (rc != 0) {
    LogError("failed to clear timestamp");
  }

  // Arm bank1 to start.
  rc = Write(0x420, 0x1);
  
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
    rc = Read(0x60, msg);
    ++count;
  } while ((rc < 0) && (count < 100));
 
  is_event = msg & 0x80000;

  if (is_event && go_time_) {
    // rearm the logic
    uint armit = 1;

    count = 0;
    rc = 0;
    do {
      if (bank2_armed_flag) {

	rc = Write(0x420, armit);
	bank2_armed_flag = false;

      } else {

	rc = Write(0x424, armit);
	bank2_armed_flag = true;
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
  uint trace_addr, addr, offset, msg;

  // Check how long the event is.
  uint next_sample_address[SIS_3316_CH];
  static uint data[SIS_3316_CH][3 + SIS_3316_LN / 2];

  // Get the system time.
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  // Now get the raw data (timestamp and waveform).
  for (ch = 0; ch < SIS_3316_CH; ch++) {

    // Calculate the register for previous address.
    offset = 0x1120 + kAdcRegOffset * (ch / SIS_3316_GR);
    offset += 0x4 * (ch % SIS_3316_GR);

    // Read out the previous address.
    count = 0;
    do {

      rc = Read(offset, msg);

      if (rc != 0) {
	LogError("failure reading address for channel %i", ch);
      }

      if (count++ > 10000) {
	LogError("read event timed out");
	return;
      }

    } while ((msg & 0x1000000) != (!bank2_armed_flag << 24));

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
    addr = 0x80 + 0x4 * (ch / SIS_3316_GR);
    rc = Write(addr, msg);

    if (rc != 0) {
      LogError("failed begin data tranfer for channel %i", ch);
    }
    usleep(5);

    // Set to the adc base memory.
    count = 0;
    trace_addr = 0x100000 * (ch / SIS_3316_GR + 1);
    LogMessage("attempting to read trace at 0x%08x", trace_addr);

    do {
      rc = ReadTraceMblt64Fifo(trace_addr, data[ch]);
    } while ((rc < 0) && (count++ < 100));

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
}

int WorkerSis3316::I2cStart(int osc)
{
  if (osc < 0 || osc > 3) {
    LogError("I2C oscillator out of range, 0-3");
    return 1;
  }

  uint addr = 0, msg = 0, count = 0;

  // Set address to specified oscillator register (0 for int, 1|2 for ext).
  addr = 0x40 + 0x4 * osc;

  // Write to START bit.
  if (Write(addr, 0x1 << 9) != 0) {
    LogError("failed to start I2C oscillator 0");
  }

  // Read until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
  } while ((count++ < 1000) && ((msg >> 31) & 0x1));

  if (count >= 1000) return 2;

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
  addr = 0x40 + 0x4 * osc;
  
  // Write to STOP bit.
  if (Write(addr, 0x1 << 11) != 0) {
    LogError("failed to stop I2C oscillator 0");
  }

  // Read until no longer busy.
  do {
    if (Read(addr, msg) != 0) {
      LogError("failed to read I2C register");
    }
  } while ((count++ < 1000) && ((msg >> 31) & 0x1));

  
  if (count >= 1000) {
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
  addr = 0x40 + 0x4 * osc;

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
  } while ((count++ < 1000) && ((msg >> 31) & 0x1));

  if (count >= 1000) {
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
  addr = 0x40 + 0x4 * osc;

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
  } while ((count++ < 1000) && ((msg >> 31) & 0x1));

  if (count >= 1000) {
    LogError("I2C busy timed out");
    return 2;
  }
  
  // Check the ACK bit.
  ack = (msg >> 8) & 0x1;

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
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
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
  rc = I2cWrite(osc, 0x40, ack);
  if ((rc != 0) || (!ack)) {
    LogError("failure writing I2C byte");
    I2cStop(osc);
  }  

  // And stop command.
  rc = I2cStop(osc);
  
  // Wait, then reset the DCM.
  usleep(15000);
  rc = Write(0x438, 0x0);
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
  addr = 0x100c + gr * 0x1000;

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
  } while ((rc != 0) && (count++ < maxcount));
  
  if (count >= maxcount) {
    return -2;
  }
  
  return rc;
}

} // ::daq
