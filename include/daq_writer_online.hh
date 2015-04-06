#ifndef SLAC_DAQ_INCLUDE_DAQ_WRITER_ONLINE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WRITER_ONLINE_HH_

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <fstream>
#include <queue>
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <zmq.hpp>
#include <json_spirit.h>

//--- project includes ------------------------------------------------------//
#include "daq_writer_base.hh"
#include "daq_structs.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class DaqWriterOnline : public DaqWriterBase {

  public:

    //ctor
    DaqWriterOnline(string conf_file);
  
    //dtor
    ~DaqWriterOnline() {
      // Dump data.
      go_time_ = false;
      FlushData();

      // Kill thread.
      thread_live_ = false;
      if (writer_thread_.joinable()) {
        writer_thread_.join();
      }
    };

    // Member Functions
    void LoadConfig();
    void StartWriter() { 
        go_time_ = true; 
        number_of_events_ = 0; };
    void StopWriter() { go_time_ = false; };

    void PushData(const vector<event_data> &data_buffer);
    void EndOfBatch(bool bad_data);

  private:

    const int kMaxQueueSize = 5;
    int max_trace_length_;
    int number_of_events_;
    std::atomic<bool> message_ready_;
    std::atomic<bool> go_time_;
    std::atomic<bool> queue_has_data_;
    std::queue<event_data> data_queue_;

    // zmq stuff
    zmq::context_t online_ctx_;
    zmq::socket_t online_sck_;
    zmq::message_t message_;

    void PackMessage();
    void SendMessageLoop();
    void FlushData() {
      writer_mutex_.lock();
      while (!data_queue_.empty()) {
	data_queue_.pop();
      }
      queue_has_data_ = false;
      writer_mutex_.unlock();
    };
};

} // ::daq

#endif
