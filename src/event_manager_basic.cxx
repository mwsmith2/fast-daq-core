#include "event_manager_basic.hh"

namespace daq {

  EventManagerBasic::EventManagerBasic() : EventManagerBase()
{
  go_time_ = false;
  thread_live_ = false;

  conf_file_ = std::string("config/fe_vme_basic.json");
}

EventManagerBasic::~EventManagerBasic() 
{
}

int EventManagerBasic::BeginOfRun() 
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  for (auto &v : conf.get_child("devices.sis_3302")) {

    std::string name(v.first);
    std::string dev_conf_file(v.second.data());

    daq_workers_.PushBack(new DaqWorkerSis3302(name, dev_conf_file));
  }

  for (auto &v : conf.get_child("devices.sis_3350")) {

    std::string name(v.first);
    std::string dev_conf_file(v.second.data());

    daq_workers_.PushBack(new DaqWorkerSis3350(name, dev_conf_file));
  }

  max_event_time_ = conf.get<int>("max_event_time", 1000);

  thread_live_ = true;
  run_thread_ = std::thread(&EventManagerBasic::RunLoop, this);

  go_time_ = true;
  daq_workers_.StartRun();
  
  // Pop stale events
  while (!daq_workers_.AnyWorkersHaveEvent());
  daq_workers_.FlushEventData();
}

int EventManagerBasic::EndOfRun() 
{
  go_time_ = false;
  thread_live_ = false;

  daq_workers_.StopRun();

  // Try to join the thread.
  if (run_thread_.joinable()) {
    run_thread_.join();
  }

  daq_workers_.FreeList();

  return 0;
}

int EventManagerBasic::ResizeEventData(event_data &data) 
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int count = 0;
  for (auto &dev : conf.get_child("devices.sis_3302")) {
    count++;
  }
  data.sis_slow.resize(count);

  count = 0;
  for (auto &dev : conf.get_child("devices.sis_3350")) {
    count++;
  }
  data.sis_fast.resize(count);

  return 0;
}

void EventManagerBasic::RunLoop() 
{
  while (thread_live_) {
    
    daq_workers_.FlushEventData();

    while (go_time_) {

      if (daq_workers_.AnyWorkersHaveEvent()) {
	
	// Wait to be sure the others have events too.
	usleep(max_event_time_);

	if (!daq_workers_.AllWorkersHaveEvent()) {

	  daq_workers_.FlushEventData();
	  continue;

	} else if (daq_workers_.AnyWorkersHaveMultiEvent()) {

	  daq_workers_.FlushEventData();
	  continue;
	}

	event_data bundle;
	daq_workers_.GetEventData(bundle);
	
	queue_mutex_.lock();
	if (data_queue_.size() <= kMaxQueueSize) {
	  data_queue_.push(bundle);
	}
	queue_mutex_.unlock();

	has_event_ = true;
	daq_workers_.FlushEventData();
      }
 
      std::this_thread::yield();
      usleep(daq::short_sleep);
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

} // ::daq
