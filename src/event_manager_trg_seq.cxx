#include "event_manager_trg_seq.hh"

namespace daq {

EventManagerTrgSeq::EventManagerTrgSeq(int num_probes) : EventManagerBase()
{
  go_time_ = false;
  thread_live_ = false;

  sequence_in_progress_ = false;
  builder_has_finished_ = true;
  mux_round_configured_ = false;

  conf_file_ = std::string("config/fe_vme_shimming.json");

  num_probes_ = num_probes;
}

EventManagerTrgSeq::EventManagerTrgSeq(std::string conf_file, int num_probes) : 
  EventManagerBase()
{
  go_time_ = false;
  thread_live_ = false;

  sequence_in_progress_ = false;
  builder_has_finished_ = true;
  mux_round_configured_ = false;

  conf_file_ = conf_file;
  num_probes_ = num_probes;
}

EventManagerTrgSeq::~EventManagerTrgSeq() 
{
  for (auto &val : mux_boards_) {
    delete val;
  }

  delete nmr_pulser_trg_;
}

int EventManagerTrgSeq::BeginOfRun() 
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  // First set the config-dir if there is one.
  conf_dir = conf.get<std::string>("config_dir", conf_dir);

  int sis_idx = 0;
  for (auto &v : conf.get_child("devices.sis_3302")) {

    std::string name(v.first);
    std::string dev_conf_file = conf_dir + std::string(v.second.data());
    sis_idx_map_[name] = sis_idx++;

    workers_.PushBack(new WorkerSis3302(name, dev_conf_file));
  }

  // Set up the NMR pulser trigger.
  char bid = conf.get<char>("devices.nmr_pulser.dio_board_id");
  int port = conf.get<int>("devices.nmr_pulser.dio_port_num");
  nmr_trg_bit_ = conf.get<int>("devices.nmr_pulser.dio_trg_bit");
  
  switch (bid) {
    case 'a':
      nmr_pulser_trg_ = new DioTriggerBoard(vme_path, 0x0, BOARD_A, port);
      break;

    case 'b':
      nmr_pulser_trg_ = new DioTriggerBoard(vme_path, 0x0, BOARD_B, port);
      break;

    case 'c':
      nmr_pulser_trg_ = new DioTriggerBoard(vme_path, 0x0, BOARD_C, port);   
      break;

    default:
      nmr_pulser_trg_ = new DioTriggerBoard(vme_path, 0x0, BOARD_D, port);  
      break;
  }

  max_event_time_ = conf.get<int>("max_event_time", 1000);
  trg_seq_file_ = conf_dir + conf.get<std::string>("trg_seq_file");
  mux_conf_file_ = conf_dir + conf.get<std::string>("mux_conf_file");

  // Load trigger sequence
  boost::property_tree::read_json(trg_seq_file_, conf);
  
  for (auto &mux : conf) {

    int count = 0;
    for (auto &chan : mux.second) {

      // If we are adding a new mux, resize.
      if (trg_seq_.size() <= count) {
	trg_seq_.resize(count + 1);
      }
      
      // Map the multiplexer configuration.
      std::pair<std::string, int> trg(mux.first, std::stoi(chan.first));
      trg_seq_[count++].push_back(trg);

      // Map the data type and array index.
      for (auto &data : chan.second) {
	int idx = std::stoi(data.second.data());
	std::pair<std::string, int> loc(data.first, idx);
	data_out_[trg] = loc;
      }
    }
  }

  mux_boards_.resize(0);
  mux_boards_.push_back(new MuxControlBoard(vme_path, 0x0, BOARD_A));
  mux_boards_.push_back(new MuxControlBoard(vme_path, 0x0, BOARD_B));
  mux_boards_.push_back(new MuxControlBoard(vme_path, 0x0, BOARD_C));
  mux_boards_.push_back(new MuxControlBoard(vme_path, 0x0, BOARD_D));
 
  std::map<char, int> bid_map;
  bid_map['a'] = 0;
  bid_map['b'] = 1;
  bid_map['c'] = 2;
  bid_map['d'] = 3;

  // Load the data channel/mux maps
  boost::property_tree::read_json(mux_conf_file_, conf);
  for (auto &mux : conf) {
    char bid = mux.second.get<char>("dio_board_id");
    std::string mux_name(mux.first);
    int port = mux.second.get<int>("dio_port_num");

    mux_idx_map_[mux_name] = bid_map[bid];
    mux_boards_[mux_idx_map_[mux_name]]->AddMux(mux_name, port);

    std::string wfd_name(mux.second.get<std::string>("wfd_name"));
    int wfd_chan(mux.second.get<int>("wfd_chan"));    
    std::pair<std::string, int> data_map(wfd_name, wfd_chan);

    data_in_[mux.first] = data_map;
  }

  // Start threads
  thread_live_ = true;
  run_thread_ = std::thread(&EventManagerTrgSeq::RunLoop, this);
  trigger_thread_ = std::thread(&EventManagerTrgSeq::TriggerLoop, this);
  builder_thread_ = std::thread(&EventManagerTrgSeq::BuilderLoop, this);
  starter_thread_ = std::thread(&EventManagerTrgSeq::StarterLoop, this);

  go_time_ = true;
  workers_.StartRun();
  
  // Pop stale events
  while (workers_.AnyWorkersHaveEvent()) {
    workers_.FlushEventData();
  }
}

int EventManagerTrgSeq::EndOfRun() 
{
  go_time_ = false;
  thread_live_ = false;

  workers_.StopRun();

  // Try to join the threads.
  if (run_thread_.joinable()) {
    run_thread_.join();
  }

  if (trigger_thread_.joinable()) {
    trigger_thread_.join();
  }

  if (builder_thread_.joinable()) {
    builder_thread_.join();
  }

  if (starter_thread_.joinable()) {
    starter_thread_.join();
  }

  workers_.FreeList();

  return 0;
}

int EventManagerTrgSeq::ResizeEventData(event_data &data) 
{
  return 0;
}

void EventManagerTrgSeq::RunLoop() 
{
  while (thread_live_) {
    
    workers_.FlushEventData();

    while (go_time_) {
      
      if (workers_.AnyWorkersHaveEvent()) {
        
        LogMessage("RunLoop: got potential event");
        
        // Wait to be sure the others have events too.
        usleep(max_event_time_);
        
        if (!workers_.AllWorkersHaveEvent()) {
          
          workers_.FlushEventData();
          continue;
          
        } else if (workers_.AnyWorkersHaveMultiEvent()) {
          
          workers_.FlushEventData();
          continue;
        }
        
        event_data bundle;
        workers_.GetEventData(bundle);
        
        queue_mutex_.lock();
        if (data_queue_.size() <= kMaxQueueSize) {
          data_queue_.push(bundle);
          LogMessage(std::string("RunLoop: Got data. Data queue is now: ") +
                   std::to_string(data_queue_.size()));
        }
        
        queue_mutex_.unlock();
        workers_.FlushEventData();
      }
      
      std::this_thread::yield();
      usleep(daq::short_sleep);
    }
    
    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

void EventManagerTrgSeq::TriggerLoop()
{
  while (thread_live_) {

    while (go_time_) {

      if (got_start_trg_) {
	
	sequence_in_progress_ = true;
	builder_has_finished_ = false;
	mux_round_configured_ = false;

	LogMessage("TriggerLoop: beginning trigger sequence");

	for (auto &round : trg_seq_) { // {mux_conf_0...mux_conf_n}
	  if (!go_time_) break;

	  got_round_data_ = false;

	  for (auto &conf : round) { // {mux_name, set_channel}
	    if (!go_time_) break;

	    LogMessage(std::string("TriggerLoop: setting ") + 
		     conf.first + std::string(" to ") +
		     std::to_string(conf.second));

	    auto mux_board = mux_boards_[mux_idx_map_[conf.first]];
	    mux_board->SetMux(conf.first, conf.second);
	  }

	  LogMessage("TriggerLoop: muxes are configured for this round");

	  nmr_pulser_trg_->FireTrigger(nmr_trg_bit_);
	  mux_round_configured_ = true;
	  
	  while (!got_round_data_ && go_time_) {
	    std::this_thread::yield();
	    usleep(daq::short_sleep);
	  } 

	} // on to the next round

	sequence_in_progress_ = false;
	got_start_trg_ = false;

	// Pause to copy data.
	while (!builder_has_finished_ && go_time_) {
	  std::this_thread::yield();
	  usleep(daq::short_sleep);
	};

      } // done with trigger sequence

      std::this_thread::yield();
      usleep(daq::short_sleep);
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

void EventManagerTrgSeq::BuilderLoop()
{
  while (thread_live_) {
    
    std::vector<double> tm(NMR_FID_LN, 0.0);
    std::vector<double> wf(NMR_FID_LN, 0.0);

    for (int i = 0; i < NMR_FID_LN; ++i) {
      tm[i] = i * sample_period;
    }
    
    while (go_time_) {

      static nmr_data bundle;
      if (bundle.sys_clock.size() < num_probes_) bundle.Resize(num_probes_);

      static event_data data;
      int seq_index = 0;

      while (sequence_in_progress_ && go_time_) {

        if (mux_round_configured_) {
	
          if (!data_queue_.empty()) {
            queue_mutex_.lock();
            data = data_queue_.front();
            data_queue_.pop();
            queue_mutex_.unlock();

            LogMessage("BuilderLoop: Got data, starting to copy it");

            for (auto &pair : trg_seq_[seq_index]) {
	      
              // Get the right data out of the input.
              int sis_idx = sis_idx_map_[data_in_[pair.first].first];
              int trace_idx = data_in_[pair.first].second;

              LogMessage(std::string("BuilderLoop: Copied sis ") +
                       std::to_string(sis_idx) +
                       std::string(", ch ") +
                       std::to_string(trace_idx));

              auto trace = data.sis_slow[sis_idx].trace[trace_idx];
              auto clock = data.sis_slow[sis_idx].device_clock[trace_idx];

              // Store index
              int idx = data_out_[pair].second;
	      
              // Get FID data
              auto arr_ptr = &bundle.trace[idx][0];
              std::copy(&trace[0], &trace[SIS_3302_LN], arr_ptr);
              std::copy(&trace[0], &trace[SIS_3302_LN], wf.begin());
	      
              // Get the timestamp
              struct timeval tv;
              gettimeofday(&tv, nullptr);
              bundle.sys_clock[idx] = tv.tv_sec + tv.tv_usec / 1.0e6;
              bundle.gps_clock[idx] = 0.0; // todo:
              bundle.dev_clock[idx] = clock;
              
              // Extract the FID frequency and some diagnostic params.
              fid::FID myfid(tm, wf);

              // Make sure we got an FID signal
              if (myfid.isgood()) {

                bundle.snr[idx] = myfid.snr();
                bundle.len[idx] = myfid.fid_time();
                bundle.freq[idx] = myfid.CalcPhaseFreq();
                bundle.ferr[idx] = myfid.freq_err();
                bundle.method[idx] = (ushort)fid::Method::ZC;
                bundle.health[idx] = myfid.isgood();
                bundle.freq_zc[idx] = myfid.CalcZeroCountFreq();
                bundle.ferr_zc[idx] = myfid.freq_err();

              } else {

                myfid.PrintDiagnosticInfo();
                bundle.snr[idx] = 0.0;
                bundle.len[idx] = 0.0;
                bundle.freq[idx] = 0.0;
                bundle.ferr[idx] = 0.0;
                bundle.method[idx] = (ushort)fid::Method::ZC;
                bundle.health[idx] = myfid.isgood();
                bundle.freq_zc[idx] = 0.0;
                bundle.ferr_zc[idx] = 0.0;
              }
            } // next pair
	    
            seq_index++;
            got_round_data_ = true;
            mux_round_configured_ = false;
          }
        } // next round

        std::this_thread::yield();
        usleep(daq::short_sleep);
      }

      // Sequence finished.
      if (!sequence_in_progress_ && !builder_has_finished_) {

        LogMessage("BuilderLoop: Pushing event to run_queue_");
        
        queue_mutex_.lock();
        run_queue_.push(bundle);
        
        LogMessage(std::string("BuilderLoop: Size of run_queue_ = ") +
                 std::to_string(run_queue_.size()));
        
        has_event_ = true;
        seq_index = 0;
        builder_has_finished_ = true;
        queue_mutex_.unlock();
      }
      
      std::this_thread::yield();
      usleep(daq::long_sleep);
    } // go_time_
    
    std::this_thread::yield();
    usleep(daq::long_sleep);
  } // thread_live_
}

void EventManagerTrgSeq::StarterLoop()
{
  // Set up the zmq socket.
  zmq::socket_t trigger_sck(msg_context, ZMQ_SUB);
  zmq::message_t msg(64);
  bool rc = false;

  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  // Set up the SUB socket.
  std::string sck_addr = conf.get<std::string>("master_address");
  sck_addr += std::string(":");
  sck_addr += conf.get<std::string>("trigger_port");

  int opt = 1000;
  trigger_sck.setsockopt(ZMQ_RCVTIMEO, &opt, sizeof(opt));
  trigger_sck.connect(sck_addr.c_str());
  trigger_sck.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  while (thread_live_) {
    
    while (go_time_ && thread_live_) {

      // Check for zmq socket trigger.
      rc = trigger_sck.recv(&msg, ZMQ_DONTWAIT);

      if (rc == true) {
	got_start_trg_ = true;
	LogMessage("StarterLoop: Got tcp start trigger");
      }

      if (got_software_trg_){
	got_start_trg_ = true;
	got_software_trg_ = false;
	LogMessage("StarterLoop: Got software trigger");
      }

      std::this_thread::yield();
      usleep(daq::long_sleep);
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

} // ::daq


