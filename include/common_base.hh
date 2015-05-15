#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_BASE_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_BASE_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   common_base.hh
  
  about:  Creates a virtual base class for all daq classes to inherit from.
         Implements a logging scheme.
          
\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <string>
#include <fstream>
#include <cstdarg>

//--- project includes ------------------------------------------------------//

namespace daq {

class CommonBase {

 public:

  // Ctor params:
  //   name - used in logging output
  CommonBase(std::string name) : name_(name) {};
  ~CommonBase() {};
  
 protected:
  
  std::string name_; // given class(hardware) name
  char logstr_[512];
  time_t timer_;

  static bool logging_on_; 
  static std::string logfile_;
  static std::ofstream logstream_;

  inline int LogMessage(const char *format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };
  
  inline int LogMessage(const std::string& format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format.c_str());
      vsprintf(logstr_, format.c_str(), args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };
  
  inline int LogWarning(const char *format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << "[WARNING]: " << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };

  inline int LogWarning(const std::string& format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << "[WARNING]: " << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format.c_str());
      vsprintf(logstr_, format.c_str(), args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };

  inline int LogError(const char *format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << "[ERROR!]: " << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };

  inline int LogError(const std::string& format, ...) {
    
    if (logging_on_) {

      time(&timer_);
      auto t = localtime(&timer_);
      logstream_ << "[ERROR!]: " << name_ << " [" << asctime(t) << "]\n";
      
      va_list args;
      va_start(args, format.c_str());
      vsprintf(logstr_, format.c_str(), args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      return 0;
    }
  };

  void SetLogFile(const std::string& logfile) {
   
    logfile_ = logfile;
    
    if (logstream_.is_open()) {
      logstream_.close();
    }

    logstream_.open(logfile);
  };

};
  
} // daq

#endif
