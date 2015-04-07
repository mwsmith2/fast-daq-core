#ifndef DAQ_FAST_CORE_INCLUDE_DAQ_WRITER_ROOT_HH_
#define DAQ_FAST_CORE_INCLUDE_DAQ_WRITER_ROOT_HH_

//--- std includes ----------------------------------------------------------//
#include <iostream>

//--- other includes --------------------------------------------------------//
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "daq_writer_base.hh"
#include "daq_common.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class DaqWriterRoot : public DaqWriterBase {

 public:

  //ctor
  DaqWriterRoot(std::string conf_file);
  
  // Member Functions
  void LoadConfig();
  void StartWriter();
  void StopWriter();
  
  void PushData(const vector<event_data> &data_buffer);
  void EndOfBatch(bool bad_data);
  
 private:
  
  bool need_sync_;
  std::string outfile_;
  std::string tree_name_;
  
  TFile *pf_;
  TTree *pt_;

  event_data root_data_;
};

} // ::daq

#endif
