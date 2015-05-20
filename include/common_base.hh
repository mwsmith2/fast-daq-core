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
#include <iomanip>
#include <cstring>
#include <cstdarg>
#include <mutex>

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
  static std::mutex log_mutex_;

  inline int LogMessage(const char *format, ...) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "]: ";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      log_mutex_.unlock();      
      
      return 0;
    }
  };
  
  inline int LogMessage(const std::string& message) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "]: ";
      logstream_ << message << std::endl;
      
      log_mutex_.unlock();      
      return 0;
    }
  };
  
  inline int LogWarning(const char *format, ...) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "][WARNING]: ";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      log_mutex_.unlock();      
      return 0;
    }
  };

  inline int LogWarning(const std::string& warning, ...) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "][WARNING]: ";
      logstream_ << warning << std::endl;
      
      log_mutex_.unlock();      
      return 0;
    }
  };

  inline int LogError(const char *format, ...) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "][ERROR!]: ";

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      log_mutex_.unlock();      
      return 0;
    }
  };

  inline int LogError(const std::string& error) {
    
    if (logging_on_) {
      log_mutex_.lock();      

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(20) << name_;
      logstream_ << " [" << tm_str << "][ERROR!]: ";
      logstream_ << error << std::endl;
      
      log_mutex_.unlock();      
      return 0;
    }
  };

  void SetLogFile(const std::string& logfile) {
   
    log_mutex_.lock();      
    logfile_ = logfile;
    
    if (logstream_.is_open()) {
      logstream_.close();
    }

    logstream_.open(logfile);

    log_mutex_.unlock();      
  };

};
  
} // daq

#endif
