#include "daq_writer_midas.hh"

namespace daq {

  DaqWriterMidas::DaqWriterMidas(string conf_file) : DaqWriterBase(conf_file), midas_ctx_(1), midas_rep_sck_(midas_ctx_, ZMQ_REP), midas_data_sck_(midas_ctx_, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  get_next_event_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&DaqWriterMidas::SendMessageLoop, this);
}

void DaqWriterMidas::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  int hwm = conf.get<int>("writers.midas.high_water_mark", 10);
  int linger = 0;
  midas_rep_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  midas_rep_sck_.bind(conf.get<string>("writers.midas.req_port").c_str());

  midas_data_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  midas_data_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  midas_data_sck_.bind(conf.get<string>("writers.midas.data_port").c_str());
}

void DaqWriterMidas::PushData(const vector<event_data> &data_buffer)
{
  // Grab only the most recent event

  writer_mutex_.lock();
  while (!data_queue_.empty()) {
    data_queue_.pop();
  }

  if ((data_buffer.size() != 0) && get_next_event_) {
    data_queue_.push(data_buffer[data_buffer.size()-1]);
    queue_has_data_ = true;
  }
  writer_mutex_.unlock();
  cout << "Midas writer was pushed some data." << endl;
}

void DaqWriterMidas::EndOfBatch(bool bad_data)
{
  // while (!data_queue_.empty()) {
  //   SendDataMessage();
  // }

  // zmq::message_t msg(10);
  // memcpy(msg.data(), string("__EOB__").c_str(), 10);

  // int count = 0;
  // while (count < 10) {

  //   midas_sck_.send(msg, ZMQ_DONTWAIT);
  //   usleep(100);

  //   count++;
  // }
}

void DaqWriterMidas::SendMessageLoop()
{
  ptree conf;
  read_json(conf_file_, conf);

  bool rc = false;
  zmq::message_t req_msg(20);

  while (thread_live_) {

    while (go_time_) {

      do {
	rc = midas_rep_sck_.recv(&req_msg, ZMQ_NOBLOCK);
      } while ((rc == false) && (zmq_errno() == EAGAIN));
      
      do {
	rc = midas_rep_sck_.send(req_msg, ZMQ_NOBLOCK);
      } while ((rc == false) && (zmq_errno() == EAGAIN));
      
      if (rc == true) {
	get_next_event_ = true;
	usleep(100);
	SendDataMessage();
      }
  
      usleep(daq::kShortSleep);
      std::this_thread::yield();
    }
    
    usleep(daq::kLongSleep);
    std::this_thread::yield();
  }
}

void DaqWriterMidas::SendDataMessage()
{
  using boost::uint64_t;

  cout << "Midas writer started sending data." << endl;

  int count = 0;
  bool rc = false;
  char str[50];
  string data_str;

  // Make sure there is an event
  while(!queue_has_data_) {
    usleep(100);
  }

  cout << "Queue got data." << endl;
  // Copy the first event
  writer_mutex_.lock();
  event_data data = data_queue_.front();
  data_queue_.pop();

  if (data_queue_.size() == 0) queue_has_data_ = false;
  get_next_event_ = false;
  writer_mutex_.unlock();

  zmq::message_t som_msg(10);
  string msg("__SOM__");
  memcpy(som_msg.data(), msg.c_str(), msg.size()); 

  do {
    midas_data_sck_.send(som_msg, ZMQ_SNDMORE);
  } while ((rc == false) && (zmq_errno() == EINTR));

  // For each device send "<dev_name>:<dev_type>:<binary_data>".
  
  count = 0;
  for (auto &sis : data.sis_fast) {
    
    sprintf(str, "sis_fast_%i:", count++);
    data_str.append(string(str));
    data_str.append("sis_3350:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &sis, sizeof(sis));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }
  
  count = 0;
  for (auto &sis : data.sis_slow) {

    sprintf(str, "sis_slow_%i:", count++);
    data_str.append(string(str));
    data_str.append("sis_3302:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &sis, sizeof(sis));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  count = 0;
  for (auto &caen : data.caen_adc) {

    sprintf(str, "caen_adc_%i:", count++);
    data_str.append(string(str));
    data_str.append("caen_1785:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(caen));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &caen, sizeof(caen));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  count = 0;
  for (auto &caen : data.caen_drs) {

    sprintf(str, "caen_drs_%i:", count++);
    data_str.append(string(str));
    data_str.append("caen_6742:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(caen));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &caen, sizeof(caen));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  zmq::message_t eom_msg(10);
  msg = string("__EOM__:");
  memcpy(eom_msg.data(), msg.c_str(), msg.size()); 
  
  do {
    rc = midas_data_sck_.send(eom_msg);
  } while ((rc == false) && (zmq_errno() == EINTR));

  cout << "Midas writer finished sending data." << endl;

}

} // ::daq
