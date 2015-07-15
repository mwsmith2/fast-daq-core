#ifndef DAQ_FAST_CORE_INCLUDE_INO_STEPPER_MOTOR_HH_
#define DAQ_FAST_CORE_INCLUDE_INO_STEPPER_MOTOR_HH_

/*============================================================================*\
Author: Ronaldo Ortez
email: supron00@gmail.com
file: duino_stepper_motor.hh

About: stepper motor class to interface with the Arduino controlled stepper 
       motor via Ethernet socket

\*============================================================================*/

//--std include---------------------------------------------------------------//
#include <string>
#include <iostream>

//--other include-------------------------------------------------------------//
#include <stdio.h> //for memset
#include <sys/socket.h> // for socket structures
#include <netdb.h> // for addrinfo struct
#include "boost/property_tree/ptree.hpp"//to load params from json file
#include "boost/property_tree/json_parser.hpp"//to load params from json file

namespace daq {

  class InoStepperMotor {

  public:

    //Constructor
    InoStepperMotor(std::string conf_file);
    //Destructor
    ~InoStepperMotor();
    
    //Arduino Stepper Control Functions
    int MoveInchesForward(double in=1.0);
    int MoveInchesBackward(double in=1.0);
    int MoveCmForward(double cm);
    int MoveCmBackward(double cm=1.0);

    // private:
    std::string ip_addr;//Arduino Ip address
    std::string ip_port;//Arduino Ip port
    std::string conf_file_;

    int status;
    struct addrinfo host_info; //The struct that getaddrinfo() fills
    struct addrinfo *host_info_list; //pointer to the host's linked list
    // struct sockaddr_storage their_addr; //Client address information

    int socketfd; // The initial socket file descriptor
    int new_sd; // New file descriptor for connected socket

    double steps_per_inch;
    double steps_per_cm;

    //Manipulate a socket
    void Socket();
    void Send(signed int steps =1);
    void Connect();
    void Receive();
    void Disconnect();

    //Server functions
    void Bind();
    void Listen();
    void Accept();
    
  };


}//daq

#endif
