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

    workers_.PushBack(new WorkerSis3302(name, dev_conf_file));
  }

  for (auto &v : conf.get_child("devices.sis_3350")) {

    std::string name(v.first);
    std::string dev_conf_file(v.second.data());

    workers_.PushBack(new WorkerSis3350(name, dev_conf_file));
  }

  max_event_time_ = conf.get<int>("max_event_time", 1000);

  thread_live_ = true;
  run_thread_ = std::thread(&EventManagerBasic::RunLoop, this);

  go_time_ = true;
  workers_.StartRun();
  
  // Pop stale events
  while (!workers_.AnyWorkersHaveEvent());
  workers_.FlushEventData();
}

int EventManagerBasic::EndOfRun() 
{
  go_time_ = false;
  thread_live_ = false;

  workers_.StopRun();

  // Try to join the thread.
  if (run_thread_.joinable()) {
    run_thread_.join();
  }

  workers_.FreeList();

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
    
    workers_.FlushEventData();

    while (go_time_) {

      if (workers_.AnyWorkersHaveEvent()) {
	
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
	}
	queue_mutex_.unlock();

	has_event_ = true;
	workers_.FlushEventData();
      }
 
      std::this_thread::yield();
      usleep(daq::short_sleep);
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

} // ::daq
