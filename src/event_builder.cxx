#include "event_builder.hh"

namespace daq {

EventBuilder::EventBuilder(const DaqWorkerList &daq_workers, 
                           const vector<DaqWriterBase *> daq_writers,
                           string conf_file)
{
  daq_workers_ = daq_workers;
  daq_writers_ = daq_writers;
  conf_file_ = conf_file;

  LoadConfig();

  builder_thread_ = std::thread(&EventBuilder::BuilderLoop, this);
  push_data_thread_ = std::thread(&EventBuilder::ControlLoop, this);
}

void EventBuilder::LoadConfig()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  thread_live_ = true;
  go_time_ = false;
  quitting_time_ = false;
  finished_run_ = false;

  batch_size_ = conf.get<int>("batch_size", 10);
  max_event_time_ = conf.get<int>("max_event_time", 2000);
}

void EventBuilder::BuilderLoop()
{
  // Thread can only be killed by ending the run.
  while (thread_live_) {

    // Update the reference and drop any events outside of run time.
    batch_start_ = clock();
    daq_workers_.FlushEventData();

    // Collect data while the run isn't paused, in a deadtime or finished.
    while (go_time_) {
      
      if (WorkersGotSyncEvent()) {

	// Get the data.
	event_data bundle;
	
	daq_workers_.GetEventData(bundle);
	
	// Push it back to pull_data queue.
	queue_mutex_.lock();
	if (pull_data_que_.size() < kMaxQueueSize) {
	  pull_data_que_.push(bundle);
	}
	queue_mutex_.unlock();

	cout << "Data queue is now size: ";
	cout << pull_data_que_.size() << endl;

	daq_workers_.FlushEventData();
      }
      
      std::this_thread::yield();
      usleep(daq::kShortSleep);

    } // go_time_

    std::this_thread::yield();
    usleep(daq::kLongSleep);

  } // thread_live_
}

void EventBuilder::ControlLoop()
{
  while (thread_live_) {

    while (go_time_) {

      if (pull_data_que_.size() >= batch_size_) {
	
	cout << "Pushing data." << endl;
	CopyBatch();
	SendBatch();
      }

      if (quitting_time_) {
	
	StopWorkers();

	CopyBatch();
	daq_workers_.FlushEventData();

	SendLastBatch();
	
	go_time_ = false;
	thread_live_ = false;
	finished_run_ = true;
      }

      std::this_thread::yield();
      usleep(daq::kShortSleep);
    }

    std::this_thread::yield();
    usleep(daq::kLongSleep);
  }
}

bool EventBuilder::WorkersGotSyncEvent() 
{
  // Check if anybody has an event.
  if (!daq_workers_.AnyWorkersHaveEvent()) return false;
  cout << "Detected a trigger." << endl;

  // Wait for all devices to get a chance to read the event.
  usleep(max_event_time_); 

  // Drop the event if not all devices got a trigger.
  if (!daq_workers_.AllWorkersHaveEvent()) {
 
    daq_workers_.FlushEventData();
    cout << "Event was not synched." << endl;
    return false;
  }
    
  // Drop the event if any devices got two triggers.
  if (daq_workers_.AnyWorkersHaveMultiEvent()) {

    daq_workers_.FlushEventData();
    cout << "Was actually double." << endl;
    return false;
  }

  // Return true if we passed all other checks.
  return true;
}

void EventBuilder::CopyBatch() 
{
  push_data_mutex_.lock();
  push_data_vec_.resize(0);
  
  for (int i = 0; i < batch_size_; ++i) {
    
    queue_mutex_.lock();

    if (!pull_data_que_.empty()) {
      push_data_vec_.push_back(pull_data_que_.front());
      pull_data_que_.pop();

      cout << "EventBuilder: pull queue size = ";
      cout << pull_data_que_.size() << endl;
    }

    queue_mutex_.unlock();
  }

  push_data_mutex_.unlock();
}

void EventBuilder::SendBatch()
{
  push_data_mutex_.lock();

  for (auto &writer : daq_writers_) {
    writer->PushData(push_data_vec_);
  }

  push_data_mutex_.unlock();
}

void EventBuilder::SendLastBatch()
{
  cout << "EventBuilder: Sending last batch" << endl;

  // Zero the pull data vec
  queue_mutex_.lock();
  while (pull_data_que_.size() != 0) pull_data_que_.pop();
  queue_mutex_.unlock();

  push_data_mutex_.lock();
  cout << "Sending end of batch/run to the writers." << endl;
  for (auto &writer : daq_writers_) {
    
    writer->PushData(push_data_vec_);
    writer->EndOfBatch(false);
  }
  push_data_mutex_.unlock();
}
  

// Start the workers taking data.
void EventBuilder::StartWorkers() {
    cout << "Event Builder is starting workers." << endl;
  daq_workers_.StartWorkers();
}

// Write the data file and reset workers.
void EventBuilder::StopWorkers() {
  cout << "Event Builder is stopping workers." << endl;
  daq_workers_.StopWorkers();
}

} // ::daq
