#include "daq_writer_online.hh"

namespace daq {

DaqWriterOnline::DaqWriterOnline(std::string conf_file) : DaqWriterBase(conf_file), online_ctx_(1), online_sck_(online_ctx_, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&DaqWriterOnline::SendMessageLoop, this);
}

void DaqWriterOnline::LoadConfig()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int hwm = conf.get<int>("writers.online.high_water_mark", 10);
  online_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  int linger = 0;
  online_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  online_sck_.connect(conf.get<std::string>("writers.online.port").c_str());

  max_trace_length_ = conf.get<int>("writers.online.max_trace_length", -1);
}

void DaqWriterOnline::PushData(const std::vector<event_data> &data_buffer)
{
  WriteLog("WriterOnline: received some data.");

  writer_mutex_.lock();

  number_of_events_ += data_buffer.size();

  auto it = data_buffer.begin();
  while (data_queue_.size() < kMaxQueueSize && it != data_buffer.end()) {

    data_queue_.push(*it);
    ++it;

  }
  queue_has_data_ = true;
  writer_mutex_.unlock();
}

void DaqWriterOnline::EndOfBatch(bool bad_data)
{
  FlushData();

  zmq::message_t msg(10);
  memcpy(msg.data(), std::string("__EOB__").c_str(), 10);

  int count = 0;
  while (count < 50) {

    online_sck_.send(msg, ZMQ_DONTWAIT);
    usleep(100);

    count++;
  }
}

void DaqWriterOnline::SendMessageLoop()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  while (thread_live_) {

    while (go_time_ && queue_has_data_) {

      if (!message_ready_) {
        PackMessage();
      }

      while (message_ready_ && go_time_) {
	
	int count = 0;
	bool rc = false;
	while (rc == false && count < 200) {
	  rc = online_sck_.send(message_, ZMQ_DONTWAIT);
	  count++;
	}

        if (rc == true) {

          WriteLog("WriterOnline: sent message successfully.");
          message_ready_ = false;

        }

        usleep(daq::short_sleep);
        std::this_thread::yield();
      }
      
      usleep(daq::short_sleep);
      std::this_thread::yield();
      
    }
    
    usleep(daq::long_sleep);
    std::this_thread::yield();
  }
}

void DaqWriterOnline::PackMessage()
{
  using boost::uint64_t;

  WriteLog("WriterOnline: packing message.");

  int count = 0;
  char str[50];

  json_spirit::Object json_map;

  std::unique_lock<std::mutex> pack_message_lock(writer_mutex_);

  if (data_queue_.empty()) {
    return;
  }

  event_data data = data_queue_.front();
  data_queue_.pop();
  if (data_queue_.size() == 0) queue_has_data_ = false;

  json_map.push_back(json_spirit::Pair("event_number", number_of_events_));

  if (max_trace_length_ < 0) {
    for (auto &sis : data.sis_fast) {
      
      json_spirit::Object sis_map;
      
      sprintf(str, "system_clock");
      sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));
      
      sprintf(str, "device_clock");
      sis_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&sis.device_clock[0], 
			    (uint64_t *)&sis.device_clock[SIS_3350_CH])));
      
      json_spirit::Array arr;
      for (int ch = 0; ch < SIS_3350_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&sis.trace[ch][0], &sis.trace[ch][SIS_3350_LN]));
	
      }
      
      sprintf(str, "trace");
      sis_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "sis_fast_%i", count++);
      json_map.push_back(json_spirit::Pair(str, sis_map));
    }
    
    count = 0;
    for (auto &sis : data.sis_slow) {
      
      json_spirit::Object sis_map;
      
      sprintf(str, "system_clock");
      sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));
      
      sprintf(str, "device_clock");
      sis_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&sis.device_clock[0], 
			    (uint64_t *)&sis.device_clock[SIS_3302_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < SIS_3302_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&sis.trace[ch][0], &sis.trace[ch][SIS_3302_LN]));

      }

      sprintf(str, "trace");
      sis_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "sis_slow_%i", count++);
      json_map.push_back(json_spirit::Pair(str, sis_map));
    }
    
    count = 0;
    for (auto &caen : data.caen_adc) {
      
      json_spirit::Object caen_map;
      
      sprintf(str, "system_clock");
      caen_map.push_back(json_spirit::Pair(str, (uint64_t)caen.system_clock));
      
      sprintf(str, "device_clock");
      caen_map.push_back(json_spirit::Pair(str, 
                         json_spirit::Array(
			   (uint64_t *)&caen.device_clock[0], 
			   (uint64_t *)&caen.device_clock[CAEN_1785_CH])));

      sprintf(str, "value");
      caen_map.push_back(json_spirit::Pair(str, 
			   json_spirit::Array(
			     &caen.value[0], 
			     &caen.value[CAEN_1785_CH])));

      sprintf(str, "caen_adc_%i", count++);
      json_map.push_back(json_spirit::Pair(str, caen_map));
    }
    
    count = 0;
    for (auto &caen : data.caen_drs) {
      
      json_spirit::Object caen_map;
      
      sprintf(str, "system_clock");
      caen_map.push_back(json_spirit::Pair(str, (uint64_t)caen.system_clock));
      
      sprintf(str, "device_clock");
      caen_map.push_back(json_spirit::Pair(str, 
			   json_spirit::Array(
			     (uint64_t *)&caen.device_clock[0], 
			     (uint64_t *)&caen.device_clock[CAEN_6742_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < CAEN_6742_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&caen.trace[ch][0], &caen.trace[ch][CAEN_6742_LN]));

      }
      
      
      sprintf(str, "trace");
      caen_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "caen_drs_%i", count++);
      json_map.push_back(json_spirit::Pair(str, caen_map));
    }
    
    count = 0;
    for (auto &board : data.drs) {
      
      json_spirit::Object drs_map;
      
      sprintf(str, "system_clock");
      drs_map.push_back(json_spirit::Pair(str, (uint64_t)board.system_clock));

      sprintf(str, "device_clock");
      drs_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&board.device_clock[0], 
			    (uint64_t *)&board.device_clock[DRS4_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < DRS4_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&board.trace[ch][0], &board.trace[ch][DRS4_LN]));

      }
      
      sprintf(str, "trace");
      drs_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "drs_%i", count++);
      json_map.push_back(json_spirit::Pair(str, drs_map));
    }

  } else {    
    
    for (auto &sis : data.sis_fast) {
      
      json_spirit::Object sis_map;
      
      sprintf(str, "system_clock");
      sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));
      
      sprintf(str, "device_clock");
      sis_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&sis.device_clock[0], 
			    (uint64_t *)&sis.device_clock[SIS_3350_CH])));
      
      json_spirit::Array arr;
      for (int ch = 0; ch < SIS_3350_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&sis.trace[ch][0], &sis.trace[ch][max_trace_length_]));
	
      }
      
      sprintf(str, "trace");
      sis_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "sis_fast_%i", count++);
      json_map.push_back(json_spirit::Pair(str, sis_map));
    }
    
    count = 0;
    for (auto &sis : data.sis_slow) {
      
      json_spirit::Object sis_map;
      
      sprintf(str, "system_clock");
      sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));
      
      sprintf(str, "device_clock");
      sis_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&sis.device_clock[0], 
			    (uint64_t *)&sis.device_clock[SIS_3302_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < SIS_3302_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&sis.trace[ch][0], &sis.trace[ch][max_trace_length_]));

      }

      sprintf(str, "trace");
      sis_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "sis_slow_%i", count++);
      json_map.push_back(json_spirit::Pair(str, sis_map));
    }
    
    count = 0;
    for (auto &caen : data.caen_adc) {
      
      json_spirit::Object caen_map;
      
      sprintf(str, "system_clock");
      caen_map.push_back(json_spirit::Pair(str, (uint64_t)caen.system_clock));
      
      sprintf(str, "device_clock");
      caen_map.push_back(json_spirit::Pair(str, 
                         json_spirit::Array(
			   (uint64_t *)&caen.device_clock[0], 
			   (uint64_t *)&caen.device_clock[CAEN_1785_CH])));

      sprintf(str, "value");
      caen_map.push_back(json_spirit::Pair(str, 
			   json_spirit::Array(
			     &caen.value[0], 
			     &caen.value[CAEN_1785_CH])));

      sprintf(str, "caen_adc_%i", count++);
      json_map.push_back(json_spirit::Pair(str, caen_map));
    }
    
    count = 0;
    for (auto &caen : data.caen_drs) {
      
      json_spirit::Object caen_map;
      
      sprintf(str, "system_clock");
      caen_map.push_back(json_spirit::Pair(str, (uint64_t)caen.system_clock));
      
      sprintf(str, "device_clock");
      caen_map.push_back(json_spirit::Pair(str, 
			   json_spirit::Array(
			     (uint64_t *)&caen.device_clock[0], 
			     (uint64_t *)&caen.device_clock[CAEN_6742_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < CAEN_6742_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&caen.trace[ch][0], &caen.trace[ch][max_trace_length_]));

      }
      
      
      sprintf(str, "trace");
      caen_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "caen_drs_%i", count++);
      json_map.push_back(json_spirit::Pair(str, caen_map));
    }
    
    count = 0;
    for (auto &board : data.drs) {
      
      json_spirit::Object drs_map;
      
      sprintf(str, "system_clock");
      drs_map.push_back(json_spirit::Pair(str, (uint64_t)board.system_clock));

      sprintf(str, "device_clock");
      drs_map.push_back(json_spirit::Pair(str, 
                          json_spirit::Array(
			    (uint64_t *)&board.device_clock[0], 
			    (uint64_t *)&board.device_clock[DRS4_CH])));

      json_spirit::Array arr;
      for (int ch = 0; ch < DRS4_CH; ++ch) {
	arr.push_back(json_spirit::Array(
			&board.trace[ch][0], &board.trace[ch][max_trace_length_]));

      }
      
      sprintf(str, "trace");
      drs_map.push_back(json_spirit::Pair(str, arr));
      
      sprintf(str, "drs_%i", count++);
      json_map.push_back(json_spirit::Pair(str, drs_map));
    }
  }

  std::string buffer = json_spirit::write(json_map);
  buffer.append("__EOM__");

  message_ = zmq::message_t(buffer.size());
  memcpy(message_.data(), buffer.c_str(), buffer.size());

  WriteLog("WriterOnline: message ready.");
  message_ready_ = true;
}


} // ::daq
