#ifndef SLAC_DAQ_INCLUDE_DAQ_WRITER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WRITER_BASE_HH_

//--- std includes ----------------------------------------------------------//
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
using std::vector;
using std::string;

//--- other includes --------------------------------------------------------//


//--- project includes ------------------------------------------------------//
#include "daq_structs.hh"

namespace daq {

// This class defines an abstract base class for data writers to inherit form.

class DaqWriterBase {

  public:

    DaqWriterBase(string conf_file) : conf_file_(conf_file), thread_live_(true) {};
    virtual ~DaqWriterBase() {
        thread_live_ = false;
        if (writer_thread_.joinable()) {
            writer_thread_.join();
        }
    };

    // Basic functions
    virtual void LoadConfig() = 0;
    virtual void StartWriter() = 0;
    virtual void StopWriter() = 0;
    virtual void EndOfBatch(bool bad_data) = 0;

    virtual void PushData(const vector<event_data> &data_buffer) = 0;

  protected:

    // Simple variables
    string conf_file_;
    std::atomic<bool> thread_live_;
    std::atomic<bool> end_of_batch_;

    // Concurrency variables
    std::thread writer_thread_;
    std::mutex writer_mutex_;

};

} // ::daq

#endif
