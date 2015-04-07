#ifndef DAQ_FAST_CORE_INCLUDE_EVENT_MANAGER_TRG_SEQ_HH_
#define DAQ_FAST_CORE_INCLUDE_EVENT_MANAGER_TRG_SEQ_HH_

/*============================================================================*\
  
  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   event_manager_trg_seq.hh

  about:  Implements an event builder that sequences each channel of
          a set of multiplexers according to a json config file
	  which defines the trigger sequence.

\*============================================================================*/


//--- std includes -----------------------------------------------------------//
#include <string>
#include <map>
#include <vector>
#include <cassert>

//--- other includes ---------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <zmq.hpp>
#include "fid.h"

//--- project includes -------------------------------------------------------//
#include "event_manager_base.hh"
#include "mux_control_board.hh"
#include "dio_trigger_board.hh"

namespace daq {

class EventManagerTrgSeq: public EventManagerBase {

public:
  
  //ctor 
  EventManagerTrgSeq(int num_probes);
  EventManagerTrgSeq(std::string conf_file, int num_probes);
  
  //dtor
  ~EventManagerTrgSeq();
  
  // Launches any threads needed and start collecting data.
  int BeginOfRun();

  // Rejoins threads and stops data collection.
  int EndOfRun();

  int ResizeEventData(vme_data &data);

  // Issue a software trigger to take another sequence.
  inline int IssueTrigger() {
    got_software_trg_ = true;
  }

  // Returns the oldest stored event.
  inline const nmr_data &GetCurrentEvent() { 
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return run_queue_.front(); 
  };
  
  // Removes the oldest event from the front of the queue.
  inline void PopCurrentEvent() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (!run_queue_.empty()) {
      run_queue_.pop();
    }
    if (run_queue_.empty()) {
      has_event_ = false;
    }
  };

private:

  const std::string name_ = "EventManagerTrgSeq";
  
  int max_event_time_;
  int num_probes_;
  std::atomic<bool> got_software_trg_;
  std::atomic<bool> got_start_trg_;
  std::atomic<bool> got_round_data_;
  std::atomic<bool> builder_has_finished_;
  std::atomic<bool> sequence_in_progress_;
  std::atomic<bool> mux_round_configured_;
  std::string trg_seq_file_;
  std::string mux_conf_file_;

  int nmr_trg_bit_;
  DioTriggerBoard *nmr_pulser_trg_;
  std::vector<MuxControlBoard *> mux_boards_;
  std::map<std::string, int> mux_idx_map_;
  std::map<std::string, int> sis_idx_map_;
  std::map<std::string, std::pair<std::string, int>> data_in_;
  std::map<std::pair<std::string, int>, std::pair<std::string, int>> data_out_;
  std::vector<std::vector<std::pair<std::string, int>>> trg_seq_;

  std::queue<nmr_data> run_queue_;
  std::mutex queue_mutex_;
  std::thread trigger_thread_;
  std::thread builder_thread_;
  std::thread starter_thread_;
  
  // Collects from data workers, i.e., direct from the waveform digitizers.
  void RunLoop();

  // Listens for sequence start signals.
  void StarterLoop();

  // Runs through the trigger sequence by configuring multiplexers for
  // several rounds and interacting with the BuilderLoop to ensure data
  // capture.
  void TriggerLoop();

  // Builds the event by selecting the proper data from each round.
  void BuilderLoop();
};

} // ::daq

#endif

