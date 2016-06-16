#include "sync_client.hh"

namespace daq {

SyncClient::SyncClient() :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ),
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  base_tcpip_ = default_tcpip_;
  base_port_ = default_port_;

  DefaultInit();
  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string address) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ),
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  base_tcpip_ = address;
  base_port_ = default_port_;

  DefaultInit();
  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string address, int port) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ),
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  base_tcpip_ = address;
  base_port_ = port;

  DefaultInit();
  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string name, std::string address) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ),
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  client_name_ = name;
  base_tcpip_ = address;
  base_port_ = default_port_;

  DefaultInit();
  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string name, std::string address, int port) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ),
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  client_name_ = name;
  base_tcpip_ = address;
  base_port_ = port;

  DefaultInit();
  InitSockets();
  LaunchThreads();
}

void SyncClient::DefaultInit()
{
  register_address_ = ConstructAddress(base_tcpip_, base_port_);
  auto uuid = boost::uuids::random_generator()();
  if (client_name_.size() == 0) {
    client_name_ = boost::uuids::to_string(uuid) + std::string(";");
  }
  name_ = std::string("SyncClient-") + client_name_.substr(0, 4);
  LogMessage("full id is %s", client_name_.c_str());

  connected_ = false;
  ready_ = false;
  sent_ready_ = false;
  got_trigger_ = false;
  thread_live_ = true;

  trigger_counter_ = 0;
}

void SyncClient::InitSockets()
{
  bool rc = false;
  int one = 1;
  int zero = 0;

  zmq::message_t msg(256);
  zmq::message_t reg_msg(client_name_.size());
  std::copy(client_name_.begin(), client_name_.end(), (char *)reg_msg.data());

  // Now the registration socket options.
  register_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  register_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  register_sck_.setsockopt(ZMQ_IMMEDIATE, &one, sizeof(one));
  register_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
  //  register_sck_.setsockopt(ZMQ_REQ_RELAXED, &one, sizeof(one));
  //  register_sck_.setsockopt(ZMQ_REQ_CORRELATE, &one, sizeof(one));

  // Set up the trigger socket options.
  trigger_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  trigger_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  trigger_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

  // Now the status socket
  status_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  status_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  status_sck_.setsockopt(ZMQ_IMMEDIATE, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_REQ_RELAXED, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_REQ_CORRELATE, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

  // Now finally the heartbeat
  heartbeat_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));


  // Connect the sockets.  The registration socket first.
  register_sck_.connect(register_address_.c_str());

  do {
    rc = register_sck_.send(reg_msg, ZMQ_DONTWAIT);

    if (rc == true) {

      do {
        rc = register_sck_.recv(&msg, ZMQ_DONTWAIT);
        usleep(10);
      } while (!rc && thread_live_);
    }

    heavy_sleep(); // Don't be too aggressive here.

  } while (!rc && thread_live_);

  connected_ = true;

  // Parse the other socket addresses from the message.
  std::string address;
  std::stringstream ss;
  ss.str((char *)msg.data());

  std::getline(ss, address, ';');
  trigger_address_ = address;

  std::getline(ss, address, ';');
  status_address_ = address;

  std::getline(ss, address, ';');
  heartbeat_address_ = address;

  LogMessage("trigger address: %s", trigger_address_.c_str());
  trigger_sck_.connect(trigger_address_.c_str());
  trigger_sck_.setsockopt(ZMQ_SUBSCRIBE, "", 0); // Subscribe post connect

  LogMessage("status address: %s", status_address_.c_str());
  status_sck_.connect(status_address_.c_str());

  LogMessage("heartbeat address; %s", heartbeat_address_.c_str());
  heartbeat_sck_.connect(heartbeat_address_.c_str());
}

void SyncClient::LaunchThreads()
{
  status_thread_ = std::thread(&SyncClient::StatusLoop, this);
  restart_thread_ = std::thread(&SyncClient::RestartLoop, this);
  heartbeat_thread_ = std::thread(&SyncClient::HeartbeatLoop, this);
}

void SyncClient::StatusLoop()
{
  LogMessage("StatusLoop launched");

  zmq::message_t msg(256);
  zmq::message_t ready_msg(32);
  zmq::message_t reg_msg(client_name_.size());

  std::string tmp("READY;");

  bool rc = false;
  long long last_contact = steadyclock_us();

  while (thread_live_) {

    // Make sure we haven't timed out.
    connected_ = (steadyclock_us() - last_contact) < trigger_timeout_;

    // 1. Make sure we are connected to the trigger.
    if (!connected_) {

      LogMessage("not connected, last response %lli us ago",
                 steadyclock_us() - last_contact);

      heavy_sleep();

    } else if (!ready_) {

      // Reset the steadyclock_us, so timeouts only occur when looking for triggers.
      last_contact = steadyclock_us();

    } else if (ready_ && !sent_ready_) {

      // Request a trigger.
      LogMessage("requesting trigger");
      rc = status_sck_.send(ready_msg, ZMQ_DONTWAIT);

      if (rc == true) {

        // Complete the handshake.
        do {
          rc = status_sck_.recv(&msg, ZMQ_DONTWAIT);
          connected_ = (steadyclock_us() - last_contact) < trigger_timeout_;
          light_sleep();
        } while (!rc && thread_live_ && connected_);

        if (rc == true) {

          last_contact = steadyclock_us();
          sent_ready_ = true;
          LogMessage("flagged ready for trigger");
        }

      } else {

        light_sleep();
      }

    } else if (ready_ && sent_ready_) {

      // Wait for the trigger
      do {
        rc = trigger_sck_.recv(&msg, ZMQ_DONTWAIT);
        connected_ = (steadyclock_us() - last_contact) < trigger_timeout_;
        light_sleep();
      } while (!rc && thread_live_ && connected_ && sent_ready_);

      // Adjust the flags
      if (rc == true) {

        ready_ = false;
        sent_ready_ = false;

        got_trigger_ = true;
        ++trigger_counter_;

        LogMessage("received trigger");
        last_contact = steadyclock_us();
      }
    }

    std::this_thread::yield();
    light_sleep();
  }
}

bool SyncClient::HasTrigger()
{
  if (got_trigger_ == true) {

    got_trigger_ = false;
    return true;

  } else {

    return false;
  }
}

// Restarts the other sockets and other thread when we lose connection.
void SyncClient::RestartLoop()
{
  LogMessage("RestartLoop launched");

  while(thread_live_) {

    if (!connected_) {

      // Make sure it's still unconnected.
      usleep(125000);
      if (connected_) {
        continue;

      } else {

        LogMessage("starting to join threads");
        // Kill the other thread.
        thread_live_ = false;
        if (status_thread_.joinable()) {
          status_thread_.join();
        }
        LogMessage("joined status_thread");

        if (heartbeat_thread_.joinable()) {
          heartbeat_thread_.join();
        }
        LogMessage("joined heartbeat_thread");

        trigger_sck_.disconnect(trigger_address_.c_str());
        register_sck_.disconnect(register_address_.c_str());
        status_sck_.disconnect( status_address_.c_str());
        heartbeat_sck_.disconnect(heartbeat_address_.c_str());

        got_trigger_ = false;
        sent_ready_ = false;
        thread_live_ = true;

        // Reinitialize sockets.
        LogMessage("reinitializing sockets");
        InitSockets();
        LogMessage("reinitialization success");
        status_thread_ = std::thread(&SyncClient::StatusLoop, this);
        heartbeat_thread_ = std::thread(&SyncClient::HeartbeatLoop, this);

      }

    } else {

      std::this_thread::yield();
      heavy_sleep();
    }
  }
}

void SyncClient::HeartbeatLoop()
{
  LogMessage("HeartbeatLoop launched");

  // Try to ping every two long sleep periods.
  while (thread_live_) {

    zmq::message_t msg(client_name_.size());
    std::copy(client_name_.begin(), client_name_.end(),(char *)msg.data());

    if(heartbeat_sck_.send(msg, ZMQ_DONTWAIT) == false) {
      LogWarning("heartbeat message failed to send");
    }

    std::this_thread::yield();
    heavy_sleep();
  }
}

} // ::sp
