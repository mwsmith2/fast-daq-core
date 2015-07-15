#ifndef DAQ_FAST_CORE_INCLUDE_DUINO_STEPPER_MOTOR_HH_
#define DAQ_FAST_CORE_INCLUDE_DUINO_STEPPER_MOTOR_HH_

/*============================================================================*\
Author: Ronaldo Ortez
email: supron00@gmail.com
file: duino_stepper_motor.hh

About: stepper motor class to interface with the Arduino controlled stepper 
       via Ethernet socket

\*============================================================================*/

//--std include---------------------------------------------------------------//
#include <string>
#include <iostream>

//--other include-------------------------------------------------------------//
#include <stdio.h> //for memset
#include <sys/socket.h> // for socket structures
#include <netdb.h> // for addrinfo struct

namespace daq {

  class DuinoStepperMotor {

  public:

    //Constructor
    DuinoStepperMotor();
    //Destructor
    ~DuinoStepperMotor();
    
    //Arduino Stepper Control Functions
    int MoveInchesForward(double in=1.0);
    int MoveInchesBackward(double in=1.0);
    int MoveCmForward(double cm=1.0);
    int MoveCmBackward(double cm=1.0);

  private:
    int status;
    struct addrinfo host_info; //The struct that getaddrinfo() fills
    struct addrinfo *host_info_list; //pointer to the host's linked list
    struct sockaddr_storage their_addr; //Client address information

    int socketfd; // The initial socket file descriptor
    int new_sd; // New file descriptor for connected socket

    //Manipulate a socket
    Socket();
    Bind();
    Listen();
    Accept();
    Send(char* msg);
    
  };


}//daq
