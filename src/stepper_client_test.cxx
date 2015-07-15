
/*============================================================================*\
Author: Ronaldo Ortez
email: supron00@gmail.com
file: stepper_client_test.cxx

About: Test the functionality of the stepper motor class.

\*============================================================================*/

//--std include---------------------------------------------------------------//
#include <string>
#include <iostream>

//Project Includes
#include "ino_stepper_motor.cxx"

using namespace daq;

int main(int argc, char **argv)
{
  //load config file
  std::string conf_file;
  
  if (argc > 1) {
    conf_file = std::string(argv[1]);
  } else {
    conf_file = std::string("Applications/uw-nmr-daq/resources/config/uw/stepper_motor.json");
  }
  
  //create ino stepper class that is able to communicate through the soccet.
  InoStepperMotor stepper(conf_file); 
 
  double cm =3.0;
  stepper.MoveCmForward(cm);
  
  //Dstructor
  stepper.~InoStepperMotor();

}
