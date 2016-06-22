#include "worker_fake.hh"

#include <iostream>
using std::cout;
using std::endl;

namespace daq {

// ctor
WorkerFake::WorkerFake(std::string name, std::string conf) :
  WorkerBase<test_struct>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3350_CH;
  len_tr_ = SIS_3350_LN;
  has_fake_event_ = false;
  LogMessage("initialization complete");
}

void WorkerFake::LoadConfig()
{
  rate_ = conf_.get<double>("rate");
  mean_ = conf_.get<double>("mean");
  sigma_ = conf_.get<double>("sigma");
}

void WorkerFake::GenerateEvent()
{
  using namespace std::chrono;
  std::default_random_engine gen;

  LogMessage("Launched Event Generator");

  // Make fake events.
  while (thread_live_) {

    while (go_time_) {

      LogMessage("Generated an event");

      event_mutex_.lock(); // mutex guard
      // Get the system time
      auto t1 = high_resolution_clock::now();
      auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();

      event_data_.system_clock = duration_cast<nanoseconds>(dtn).count();

      std::normal_distribution<double> dist(mean_, sigma_);
      event_data_.value = dist(gen);

      has_fake_event_ = true;
      event_mutex_.unlock();

      usleep(1.0e6 / rate_);
      std::this_thread::yield();
    }

    usleep(daq::long_sleep);
    std::this_thread::yield();
  }
}

void WorkerFake::GetEvent(test_struct &bundle)
{
  event_mutex_.lock();

  // Copy the data.
  bundle = event_data_;
  has_fake_event_ = false;

  event_mutex_.unlock();
}

void WorkerFake::WorkLoop()
{
  LogMessage("Launching WorkLoop Thread");

  while (thread_live_) {

    t0_ = std::chrono::high_resolution_clock::now();

    while(go_time_) {

      if (EventAvailable()) {

        test_struct bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;

        LogMessage("Pushed an event into the queue");
        queue_mutex_.unlock();

      }

      std::this_thread::yield();

      usleep(daq::short_sleep);
    }

    std::this_thread::yield();

    usleep(daq::long_sleep);
  }
}

test_struct WorkerFake::PopEvent()
{
  queue_mutex_.lock();

  // If no event return empty struct
  if (data_queue_.empty()) {
      test_struct data;
      queue_mutex_.unlock();
      return data;

  } else if (!data_queue_.empty()) {

    // Copy the data.
    test_struct data = data_queue_.front();
    data_queue_.pop();

    // Check if this is that last event.
    if (data_queue_.size() == 0) has_event_ = false;

    queue_mutex_.unlock();
    return data;
  }
}

} // daq
