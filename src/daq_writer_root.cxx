#include "daq_writer_root.hh"

namespace daq {

DaqWriterRoot::DaqWriterRoot(string conf_file) : DaqWriterBase(conf_file)
{
  end_of_batch_ = false;
  LoadConfig();
}

void DaqWriterRoot::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  outfile_ = conf.get<string>("writers.root.file", "default.root");
  tree_name_ = conf.get<string>("writers.root.tree", "t");
  need_sync_ = conf.get<bool>("writers.root.sync", false);
}

void DaqWriterRoot::StartWriter()
{
  // Allocate ROOT files
  pf_ = new TFile(outfile_.c_str(), "RECREATE");
  pt_ = new TTree(tree_name_.c_str(), tree_name_.c_str());

  // Turn off autoflush, I will check for synchronization then flush.
  pt_->SetAutoFlush(0);

  // Need to get tree names out of the config file
  ptree conf;
  read_json(conf_file_, conf);

  // For each different device we need to loop and assign branches.
  string br_name;
  char br_vars[100];

  // Count the devices, reserve memory for them, then assign an address.
  int count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3350")) {
    count++;
  }
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.fake")) {
    count++;
  }
  root_data_.sis_fast.reserve(count + 1);

  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3350")) {

    root_data_.sis_fast.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

    pt_->Branch(br_name.c_str(), &root_data_.sis_fast[count++], br_vars);

  }

  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.fake")) {

    root_data_.sis_fast.resize(count);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

    pt_->Branch(br_name.c_str(), &root_data_.sis_fast[count++], br_vars);

  }

  // Again, count, reserve then assign the slow struck.
  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {
    count++;
  }
  root_data_.sis_slow.reserve(count);

  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {

    root_data_.sis_slow.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3302_CH, SIS_3302_CH, SIS_3302_LN);

    pt_->Branch(br_name.c_str(), &root_data_.sis_slow[count++], br_vars);

  }

  // Now set up the caen adc.
  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.caen_1785")) {
    count++;
  }
  root_data_.caen_adc.reserve(count);

  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.caen_1785")) {

    root_data_.caen_adc.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:value[%i]/s", 
      CAEN_1785_CH, CAEN_1785_CH);

    pt_->Branch(br_name.c_str(), &root_data_.caen_adc[count++], br_vars);

  }

  // Now set up the caen drs.
  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.caen_6742")) {
    count++;
  }
  root_data_.caen_drs.reserve(count);

  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.caen_6742")) {

    root_data_.caen_drs.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
	    CAEN_6742_CH, CAEN_6742_CH, CAEN_6742_LN);

    pt_->Branch(br_name.c_str(), &root_data_.caen_drs[count++], br_vars);

  }

  // Now set up the drs evaluation board.
  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.drs4")) {
    count++;
  }
  root_data_.drs.reserve(count);

  count = 0;
  BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.drs4")) {

    root_data_.drs.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
	    DRS4_CH, DRS4_CH, DRS4_LN);

    pt_->Branch(br_name.c_str(), &root_data_.drs[count++], br_vars);

  }
}

void DaqWriterRoot::StopWriter()
{
  pf_->Write();
  pf_->Close();
  delete pf_;
  string cmd("chown newg2:newg2 ");
  cmd += outfile_.c_str();
  cout << cmd << endl;
  system((const char*)cmd.c_str());
}

void DaqWriterRoot::PushData(const vector<event_data> &data_buffer)
{
  for (auto it = data_buffer.begin(); it != data_buffer.end(); ++it) {

    int count = 0;
    for (auto &sis : (*it).sis_fast) {
      root_data_.sis_fast[count++] = sis;
    }

    count = 0;
    for (auto &sis : (*it).sis_slow) {
      root_data_.sis_slow[count++] = sis;
    }

    count = 0;
    for (auto &caen: (*it).caen_adc) {
      root_data_.caen_adc[count++] = caen;
    }

    count = 0;
    for (auto &caen: (*it).drs) {
      root_data_.drs[count++] = caen;
    }

    pt_->Fill();

    // Manually flush the baskets.
    //    if (pt_->GetEntries() == 1000) {
    //      pt_->FlushBaskets();
    //    }
  }

}

void DaqWriterRoot::EndOfBatch(bool bad_data)
{
  cout << "Root writer got EOB with bad_data flag = " << bad_data << endl;
  if (need_sync_ && bad_data) {
    pt_->DropBaskets();
  } else {
    pt_->FlushBaskets();
  }
}

} // ::daq
