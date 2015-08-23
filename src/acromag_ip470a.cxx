#include "acromag_ip470a.hh"

namespace daq {

void AcromagIp470a::CheckBoardId()
{
  int id_addr = 0x80;
  ushort data;
  char name[5];

  for (unsigned int i = 0; i < 5; ++i) {
    Read(id_addr + 2*i, data);
    name[i] = data & 0x00ff;
  }
  name[4] = '\0';

  LogMessage("device description: %s", name);

  Read(id_addr + 0x06, data);
  LogMessage("Acromag ID: %i", data & 0x00ff);

  Read(id_addr + 0x08, data);
  LogMessage("IP model code: %i", data & 0x00ff);
}

void AcromagIp470a::ActivateEnhancedMode() 
{
  // Must write to port 7 the following: 0x7, 0xD, 0x6, 0x12
  WriteOctet(7, (u_int8_t)0x07);
  WriteOctet(7, (u_int8_t)0x0D);
  WriteOctet(7, (u_int8_t)0x06);
  WriteOctet(7, (u_int8_t)0x12);
}

int AcromagIp470a::ReadPort(int port_id)
{
  // Read data from the specified port
  if (use_sextets_) {

    return ReadSextet(port_id);

  } else {

    return ReadOctet(port_id);
  }
}

int AcromagIp470a::ReadPort(int port_id, u_int8_t& data)
{
  // Read data from the specified port
  if (use_sextets_) {

    return ReadSextet(port_id, data);

  } else {

    return ReadOctet(port_id, data);
  }
}

int AcromagIp470a::WritePort(int port_id)
{
  // Write data to the specified port
  if (use_sextets_) {

    return WriteSextet(port_id, data_);

  } else {

    return WriteOctet(port_id, data_);
  }
}

int AcromagIp470a::WritePort(int port_id, u_int8_t data) 
{
  // Write data from the specified port
  if (use_sextets_) {

    return WriteSextet(port_id, data);

  } else {

    return WriteOctet(port_id, data);
  }
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

int AcromagIp470a::ReadOctet(int port_id)
{
  // Read data from the correct port
  u_int8_t d;
  int port_address = 2 * port_id + 1;
  int rc = Read(port_address, d);

  // Reverse the bits
  d = ((d & 0xf0) >> 4) | ((d & 0x0f) << 4);
  d = ((d & 0xcc) >> 2) | ((d & 0x33) << 2);
  d = ((d & 0xaa) >> 1) | ((d & 0x55) << 1);

  // Set the data
  data_ = d;

  return rc;
}

int AcromagIp470a::ReadOctet(int port_id, u_int8_t& data)
{
  // Read data fromt he correct port then copy
  u_int8_t d;
  int port_address = 2 * port_id + 1;
  int rc = Read(port_address, d);
  
  // Reverse the bits
  d = ((d & 0xf0) >> 4) | ((d & 0x0f) << 4);
  d = ((d & 0xcc) >> 2) | ((d & 0x33) << 2);
  d = ((d & 0xaa) >> 1) | ((d & 0x55) << 1);

  // Set the data
  data = d;
  data_ = d;

  return rc;
}

int AcromagIp470a::WriteOctet(int port_id)
{
  // Write the data to the correct port
  int port_address = 2 * port_id + 1;

  // Get the current internal data.
  u_int8_t d = data_;

  // Reverse the bits
  d = ((d & 0xf0) >> 4) | ((d & 0x0f) << 4);
  d = ((d & 0xcc) >> 2) | ((d & 0x33) << 2);
  d = ((d & 0xaa) >> 1) | ((d & 0x55) << 1);

  // And write it.
  int rc = Write(port_address, d);
  return rc;
}

int AcromagIp470a::WriteOctet(int port_id, u_int8_t data) 
{
  // Write the data to the correct port
  int port_address = 2 * port_id + 1;

  // Get the current internal data.
  data_ = data;
  u_int8_t d = data_;

  // Reverse the bits
  d = ((d & 0xf0) >> 4) | ((d & 0x0f) << 4);
  d = ((d & 0xcc) >> 2) | ((d & 0x33) << 2);
  d = ((d & 0xaa) >> 1) | ((d & 0x55) << 1);

  int rc = Write(port_address, d);
  return rc;
}

int AcromagIp470a::ReadSextet(int port_id)
{
  // Read data from the specified group of 6.
  int rc, bit_shift, bit_idx;
  int vals[2];
  u_int8_t temp;
  
  // Set the byte index for it.
  bit_idx = port_id * 6;

  // Read two bytes, and get out what we need.
  rc = ReadOctet(bit_idx / 8, temp);
  vals[0] = temp;

  rc |= ReadOctet(bit_idx / 8 + 1, temp);
  vals[1] = temp;

  // Assign the value;
  bit_shift = bit_idx % 8;
  data_ = (((vals[1] << 8) + vals[0]) >> bit_shift) & 0x3f ;
  
  return rc;
}

int AcromagIp470a::ReadSextet(int port_id, u_int8_t& data)
{
  // Read data from the specified group of 6.
  int rc, bit_shift, bit_idx;
  int vals[2];
  u_int8_t temp;
  
  // Set the byte index for it.
  bit_idx = port_id * 6;

  // Read two bytes, and get out what we need.
  rc = ReadOctet(bit_idx / 8, temp);
  vals[0] = temp;

  rc |= ReadOctet(bit_idx / 8 + 1, temp);
  vals[1] = temp;

  // Assign the value;
  bit_shift = bit_idx % 8;
  data_ = (((vals[1] << 8) + vals[0]) >> bit_shift) & 0x3f ;
  
  data = data_;
  return rc;
}

int AcromagIp470a::WriteSextet(int port_id)
{
  // Write data to the specified group of 6.
  int rc, val, bit_idx, bit_shift;
  u_int8_t temp;
  u_int8_t d = data_;

  // Set the byte index for it.
  bit_idx = port_id * 6;

  // Read the first octet port to not change the data.
  rc = ReadOctet(bit_idx / 8, temp);
  val = temp;

  rc |= ReadOctet(bit_idx / 8 + 1, temp);
  val += (temp << 8);

  // Calculate bit shift for each register.
  bit_shift = bit_idx % 8;

  // Preserve old data.
  val &= 0xffff - (0x3f << bit_shift);

  // Assign new data;
  val |= ((int)d & 0x3f) << bit_shift;

  rc |= WriteOctet(bit_idx / 8, val & 0xff);    
  rc |= WriteOctet(bit_idx / 8 + 1, (val >> 8) & 0xff);
  
  return rc;
}

int AcromagIp470a::WriteSextet(int port_id, u_int8_t data) 
{
  // Write data to the specified group of 6.
  int rc, val, bit_idx, bit_shift;
  u_int8_t temp;
  u_int8_t d = data;

  // Set the byte index for it.
  bit_idx = port_id * 6;

  // Read the first octet port to not change the data.
  rc = ReadOctet(bit_idx / 8, temp);
  val = temp;

  rc |= ReadOctet(bit_idx / 8 + 1, temp);
  val += (temp << 8);

  // Calculate bit shift for each register.
  bit_shift = bit_idx % 8;

  // Preserve old data.
  val &= 0xffff - (0x3f << bit_shift);

  // Assign new data;
  val |= ((int)d & 0x3f) << bit_shift;

  rc |= WriteOctet(bit_idx / 8, val & 0xff);    
  rc |= WriteOctet(bit_idx / 8 + 1, (val >> 8) & 0xff);
  
  return rc;
}

} // ::daq
