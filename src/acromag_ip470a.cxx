#include "acromag_ip470a.hh"

namespace daq {

void AcromagIp470a::CheckBoardId()
{
  using std::endl;

  int id_addr = 0x80;
  ushort data;
  char name[5];

  for (unsigned int i = 0; i < 5; ++i) {
    Read(id_addr + 2*i, data);
    name[i] = data & 0x00ff;
  }
  name[4] = '\0';
  
  if (logging_on) {
    logstream << name_ << " device description: " << name << endl;
  }

  Read(id_addr + 0x06, data);

  if (logging_on) {
    logstream << name_ << " Acromag ID: " << (data & 0x00ff) << endl;
  }

  Read(id_addr + 0x08, data);

  if (logging_on) {
    logstream << name_ << " IP model code: " << (data & 0x00ff) << endl;
  }
}

void AcromagIp470a::ActivateEnhancedMode() 
{
  // Must write to port 7 the following: 0x7, 0xD, 0x6, 0x12
  WritePort(7, 0x07);
  WritePort(7, 0x0D);
  WritePort(7, 0x06);
  WritePort(7, 0x12);
}

int AcromagIp470a::ReadPort(int port_id)
{
  // Read data from the correct port
  int port_address = 2*port_id + 1;
  int rc = Read(port_address, data_);
  return rc;
}

int AcromagIp470a::ReadPort(int port_id, u_int8_t& data)
{
  // Read data fromt he correct port then copy
  int port_address = 2*port_id + 1;
  int rc = Read(port_address, data_);
  data = data_;
  return rc;
}

int AcromagIp470a::WritePort(int port_id)
{
  // Write the data to the correct port
  int port_address = 2*port_id + 1;
  int rc = Write(port_address, data_);
  return rc;
}

int AcromagIp470a::WritePort(int port_id, u_int8_t data) 
{
  // Write the data to the correct port
  data_ = data;
  int port_address = 2*port_id + 1;
  int rc = Write(port_address, data_);
  return rc;
}

int AcromagIp470a::ReadBit(int port_id, int bit_id, u_int8_t& data) 
{
  // Read and return a single bit's data.
  int rc = ReadPort(port_id);
  data = (data_ >> bit_id) & 0x1;
  return rc;
}

int AcromagIp470a::WriteBit(int port_id, int bit_id, u_int8_t data)
{
  // Read the current data.
  int rc = ReadPort(port_id);

  // Set the bit.
  data_ |= data << bit_id;
  data_ &= ~(~data << bit_id);

  // Write a single bit of data.
  rc = WritePort(port_id);
  return rc;
}

} // ::daq
