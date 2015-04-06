#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <cmath>
#include <chrono>
#include <ctime>
#include <thread>
using namespace std::chrono;

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"
#include "daq_structs.hh"


// This class produces fake data to test functionality
namespace daq {

typedef sis_3350 event_struct;

class DaqWorkerFake : public DaqWorkerBase<event_struct> {

  public:

    // ctor
    DaqWorkerFake(string name, string conf);

    ~DaqWorkerFake() {
        thread_live_ = false;
        if (event_thread_.joinable()) event_thread_.join();
    };

    void StartThread() {
      thread_live_ = true;
      if (work_thread_.joinable()) work_thread_.join();
      if (event_thread_.joinable()) event_thread_.join();
      work_thread_ = std::thread(&DaqWorkerFake::WorkLoop, this);
      event_thread_ = std::thread(&DaqWorkerFake::GenerateEvent, this);
    };

    void StopThread() {
        thread_live_ = false;
        if (work_thread_.joinable()) work_thread_.join();
        if (event_thread_.joinable()) event_thread_.join();
    };

    void LoadConfig();
    void WorkLoop();
    event_struct PopEvent();

  private:

    // Fake data variables
    int num_ch_;
    int len_tr_;
    std::atomic<bool> has_fake_event_;
    double rate_;
    double jitter_;
    double drop_rate_;
    high_resolution_clock::time_point t0_;

    // Concurrent data generation.
    event_struct event_data_;
    std::thread event_thread_;
    std::mutex event_mutex_;

    bool EventAvailable() { return has_fake_event_; };
    void GetEvent(event_struct &bundle);

    // The function generates fake data.
    void GenerateEvent();
};

} // ::daq

#endif
