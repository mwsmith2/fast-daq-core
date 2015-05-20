#include "altera_cycii.hh"

namespace daq {

void AlteraCycII::CheckBoardId()
{
  using std::endl;

  int id_addr = 0x80;
  ushort data;
  char name[5];
  name[4] = '\0';

  for (unsigned int i = 0; i < 5; ++i) {
    Read(id_addr + 2*i, data);
    name[i] = data & 0xff;
  }
  
  LogMessage("Device description: %s", name);

  Read(id_addr + 0x06, data);
  LogMessage("Acromag ID: %i", data & 0x00ff);

  Read(id_addr + 0x08, data);
  LogMessage("IP Model Code: %i", data & 0x00ff);
}

int AlteraCycII::LoadHexCode(std::ifstream& in)
{
  using std::endl;
  using std::cerr;

  int rc;
  char str[256];

  // Check to make sure we are in configuration mode.
  LogMessage("Starting to load hex code");

  data_ = 0xA;
  rc = Read(0x8A, data_);
  data_ &= 0xff; // only care about the last byte
  
  LogMessage("0x8A: %02x", data_);

  if (data_ == 0x49) {

    LogError("Cyclone II device is in in user mode");
    return -1;

  } else if (data_ == 0x48) {

    // Start configuration. (Set 0x0 bit-0 to logic high and wait for change)
    data_ = 0x1;
    Write(0x0, data_);

    data_ = 0x0;
    while (!(data_ & 0x1)) {
      Read(0x0, data_);
      // printf("0x00: %02x\n", data_);
      usleep(5);
    }

    while (in.good()) {
      
      usleep(500);
      std::string line;
      std::getline(in, line);
      int rc = ParseHexRecord(line, hex_data_);
      if (rc < 0) {
	//	cout << "Record parse return code: " << rc << endl;
	continue;
      } else if (rc != 0) {
	continue;
      }
      
      for (auto it = hex_data_.begin(); it != hex_data_.end(); ++it) {

	//	printf("Writing data: %02x\n", *it);
	Write(0x2, *it);

	// Poll until ready for the next byte.
	Read(0x0, data_);
	//	printf("0x00: %02x\n", data_);
	while (!(data_ & 0x3)) {
	  Read(0x0, data_);
	  //	  printf("0x00: %02x\n", data_);
	  usleep(100);
	}

	// Monitor the FPGA status
	if (data_ & 0x4) {
	  //	  printf("Status register reads: %04x\n", data_);
	  Read(0x8A, data_);
	  //	  printf("0x8A: %02x\n", data_);
	  if ((data_ & 0xff == 0x49)) {

	    LogMessage("FPGA programming complete");

	  } else {

	    LogError("Loading code onto the The Cyclone II failed");

	  }

	  break;
	}
      }
    }
    
    Read(0x8A, data_);
    printf("0x8A: %02x\n", data_);
    if (data_ & 0xff == 0x49) {
      
      LogMessage("Programming done, return to user mode");

      return 0;

    } else {

      return -1;
    }
    
  } else {
    
    // Failed to communicate
    LogError("Failed to communicate with FPGA");

    return -1;
  }
}

int AlteraCycII::LoadHexCode(std::string filename) 
{
  // Open the file
  std::ifstream in;
  in.open(filename);
  
  return LoadHexCode(in);
}

// Parse the intel hex record and do a checksum for each line.
int AlteraCycII::ParseHexRecord(const std::string& record, 
				std::vector<u_int8_t>& data)
{
  // Compute the checksum as we go.
  uint checksum = 0;
  data.resize(0);

  // The first character should be a semi-colon.
  auto it = record.begin();
  if (*(it++) != ':') return -1;

  // Now get the size of the record.
  char byte_string[5];
  byte_string[0] = *(it++);
  byte_string[1] = *(it++);
  byte_string[2] = '\0';

  uint byte_count = std::stoul(byte_string, nullptr, 16);
  checksum += byte_count;

  // Next read the 2-byte address offset.
  byte_string[0] = *(it++);
  byte_string[1] = *(it++);
  byte_string[2] = *(it++);
  byte_string[3] = *(it++);
  byte_string[4] = '\0';

  uint address = std::stoul(byte_string, nullptr, 16);
  checksum += address & 0xff;
  checksum += (address >> 8) & 0xff;

  // Next comes the 2-byte record type.
  byte_string[0] = *(it++);
  byte_string[1] = *(it++);
  byte_string[2] = '\0';

  uint record_type = std::stoul(byte_string, nullptr, 16);
  checksum += record_type;
  
  // Finally we get to the data.
  for (int i = 0; i < byte_count; ++i) {
    byte_string[0] = *(it++);
    byte_string[1] = *(it++);
    byte_string[2] = '\0';
    data.push_back(std::stoul(byte_string, nullptr, 16));
    checksum += data[i];
  }
  
  // Get the checksum and compare to calculation.
  byte_string[0] = *(it++);
  byte_string[1] = *(it++);
  byte_string[2] = '\0';

  
  if (((~checksum + 1) & 0xff) != std::stoul(byte_string, nullptr, 16)) {

    // Return an empty vector if the checksum fails.
    return -2;

  } else {

    return record_type;
  }
}
} // ::daq

    
    

