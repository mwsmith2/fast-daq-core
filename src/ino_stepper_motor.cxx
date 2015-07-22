#include "ino_stepper_motor.hh"
#include <errno.h>
#include <chrono>
#include <thread>

namespace daq {

  InoStepperMotor::InoStepperMotor(std::string conf_file):conf_file_(conf_file)
  {
    //load config params
    boost::property_tree::ptree conf;
    boost::property_tree::read_json(conf_file_, conf);

    ip_addr = conf.get<std::string>("ip_addr");
    ip_port = conf.get<std::string>("ip_port");

    steps_per_inch = 1.0 / conf.get<double>("inches_per_step");
    steps_per_cm = steps_per_inch / 2.5400;

    //We need to make sure all the fields of pointer host_info are null
    memset(&host_info,0, sizeof host_info);

    std::cout << "Setting up the structs..."  << std::endl;
    
    host_info.ai_family = AF_UNSPEC;  // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP 
    host_info.ai_flags = AI_PASSIVE; //accept connection on any host IP's

    // Now fill up the linked list of host_info structs with host's addr
    status = getaddrinfo(ip_addr.c_str(),ip_port.c_str(), &host_info, &host_info_list);
    // getaddrinfo returns 0 on succes, other value when an error occured.
    // (translated into human readable text by the gai_gai_strerror function).
    if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status);

    //Now lets create and connect to server
    Socket();
    Connect();

    //Now lets listen for Clients
    // Listen();
    //Accept();

  }

  InoStepperMotor::~InoStepperMotor() {
    // freeaddrinfo(host_info_list);
    close(socketfd);
    //close(new_sd);
    //if( new_sd ==1) close(new_sd);
  }

  void InoStepperMotor::Socket() {
    std::cout << "Creating a socket..."  << std::endl;
    
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, 
		      host_info_list->ai_protocol);
    if (socketfd == -1)  {
      std::cout << "socket error " ;
      std::cout << gai_strerror(status)<< std::endl;
    }
  }

  void InoStepperMotor::Connect() {
    std::cout << "Connect()ing..."  << std::endl;
    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1)  {
      std::cout << "connect error" ;
      std::cout << gai_strerror(status)<< std::endl;
    }
    
    // give server some time to see connection
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  void InoStepperMotor::Receive() {
    std::cout << "Waiting to recieve data..."  << std::endl;
    ssize_t bytes_recieved;
    //char incoming_data_buffer[1000];
    bytes_recieved = recv(socketfd, incoming_data_buffer,60, 0);
    // If no data arrives, the program will just wait here until some data arrives.
    if (bytes_recieved == 0) std::cout << "host shut down." << std::endl ;
    if (bytes_recieved == -1)std::cout << "recieve error!" << std::endl ;
    std::cout << bytes_recieved << " bytes recieved :" << std::endl ;
    std::cout << incoming_data_buffer << std::endl;
  }

  void InoStepperMotor::Bind() {
    std::cout << "Binding socket..."  << std::endl;
    // we make use of the setsockopt() function to make sure the port is not in 
    //use. by a previous execution of our code. (see man page for more
    //information)
    int yes = 1;
    status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1)  std::cout << "bind error" << std::endl ;
  }

  void InoStepperMotor::Listen() {
    std::cout << "Listen()ing for connections..."  << std::endl;
    status =  listen(socketfd, 5);
    if (status == -1)  std::cout << "listen error" << std::endl ;
  }
 
  void InoStepperMotor::Accept() {
    struct sockaddr_storage their_addr; //Client address information
    socklen_t addr_size = sizeof(their_addr);
    new_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_sd == -1)
    {
        std::cout << "listen error" << std::endl ;
    }
    else
    {
        std::cout << "Connection accepted. Using new socketfd : "  <<  new_sd << std::endl;
    }
  }

  void InoStepperMotor::Send(signed int  steps) {
    std::cout << "send()ing message..."  << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // const char *msg = "Hello Arduino, this is the daq computer";
    // char msg[60];

    // convert steps from signed int to char/byte for transmission
    int len =sprintf(msg, "%d ",steps);

    
    int count =0;
    ssize_t bytes_sent;

    std::cout << "original message is: "<< steps << std::endl;

    std::cout << "modified message is:  "<< msg << std::endl;

    //bytes_sent = send(socketfd, (char*)steps, len, 0);
    bytes_sent = send(socketfd, msg, len, 0);
    std::cout<<"sent bytes: "<<(int) bytes_sent<< std::endl;

    //If bytes sent failed, lets try again for 5 times until successful
    if(bytes_sent ==-1 & count<5) {
      bytes_sent = send( socketfd, msg, len, 0);

      count++;
    }
    
    //now lets listen back for completion from Arduino
    Receive();

    std::cout << "output is: " <<std::atoi(incoming_data_buffer)<< std::endl;

    while (std::atoi(incoming_data_buffer) > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }                                                            
    
    if (std::atoi(incoming_data_buffer) ==1) std::cout << "Success motor finished\n";

    //    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  void InoStepperMotor::Disconnect() {
    // freeaddrinfo(host_info_list);
    close(socketfd);
  }
  
  int InoStepperMotor::MoveInchesForward(double in) {
    Send(in*steps_per_inch + 0.5); // to round properly

    return 0;
  }
  
  int InoStepperMotor::MoveInchesBackward(double in) {
    Send(-in*steps_per_inch + 0.5); // to round properly

    return 0;
  }

  int InoStepperMotor::MoveCmForward(double cm) {
    Send(cm*steps_per_cm + 0.5); // to round properly

    return 0;
  }

  int InoStepperMotor::MoveCmBackward(double cm) {
    Send(-cm*steps_per_cm + 0.5); // to round properly

    return 0;
  }

}//::daq
