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
  
  static const int name_width_ = 16;
  static const int time_width_ = 20;

  // These should be defined in common_extdef.hh
  static int logging_verbosity_; 
  static std::string logfile_;
  static std::fstream logstream_;
  static std::mutex log_mutex_;

  std::string name_; // given class(hardware) name
  char logstr_[512];
  time_t timer_;

  // Printf style logging function (for max verbosity).
  inline int LogDebug(const char *format, ...) {
    
    if (logging_verbosity_ > 2) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::DEBUG);      

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };
  
  // Prints a simple string to the log file (for max verbosity).
  inline int LogDebug(const std::string& message) {
    
    if (logging_verbosity_ > 2) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      }

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(name_width_) << name_;
      logstream_ << " [" << tm_str << "]:   *DEBUG*   ";
      logstream_ << message << std::endl;
      
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };
  
  // Printf style logging function (verbosity = 2).
  inline int LogMessage(const char *format, ...) {
    
    if (logging_verbosity_ > 1) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      }

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(name_width_) << name_;
      logstream_ << " [" << tm_str << "]:   MESSAGE   ";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };
  
  // Prints string to log file (verbosity = 2).
  inline int LogMessage(const std::string& message) {
    
    if (logging_verbosity_ > 1) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      }

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(name_width_) << name_;
      logstream_ << " [" << tm_str << "]:   MESSAGE   ";
      logstream_ << message << std::endl;
      
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };

  // Printf style logging function (verbosity = 1).
  inline int LogWarning(const char *format, ...) {
    
    if (logging_verbosity_ > 0) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      }

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(name_width_) << name_;
      logstream_ << " [" << tm_str << "]: **WARNING** ";
      
      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);
      
      logstream_ << logstr_ << std::endl;
      
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };

  // Prints string to log file (verbosity = 1).
  inline int LogWarning(const std::string& warning) {
    
    if (logging_verbosity_ > 0) {
      log_mutex_.lock(); 

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      }

      time(&timer_);
      auto t = localtime(&timer_);

      char *tm_str = asctime(t);
      tm_str[strlen(tm_str) - 1] = '\0';

      logstream_ << std::left << std::setw(name_width_) << name_;
      logstream_ << " [" << tm_str << "]: **WARNING** ";
      logstream_ << warning << std::endl;
      
      logstream_.close();
      log_mutex_.unlock();      
    }

    return 0;
  };

  // Printf style logging function (always).
  inline int LogError(const char *format, ...) {
    
    log_mutex_.lock(); 

    while (!logstream_.is_open()) {
      logstream_.open(logfile_, std::fstream::app | std::fstream::out);
    }
    
    time(&timer_);
    auto t = localtime(&timer_);
    
    char *tm_str = asctime(t);
    tm_str[strlen(tm_str) - 1] = '\0';
    
    logstream_ << std::left << std::setw(name_width_) << name_;
    logstream_ << " [" << tm_str << "]: ***ERROR*** ";
    
    va_list args;
    va_start(args, format);
    vsprintf(logstr_, format, args);
    va_end(args);
    
    logstream_ << logstr_ << std::endl;
  
    logstream_.close();
    log_mutex_.unlock();      

    return 0;
  };

  // Prints string to log file (always).
  inline int LogError(const std::string& error) {
    
    log_mutex_.lock();

    while (!logstream_.is_open()) {
      logstream_.open(logfile_, std::fstream::app | std::fstream::out);
    }
    
    time(&timer_);
    auto t = localtime(&timer_);
    
    char *tm_str = asctime(t);
    tm_str[strlen(tm_str) - 1] = '\0';
    
    logstream_ << std::left << std::setw(name_width_) << name_;
    logstream_ << " [" << tm_str << "]: ***ERROR*** ";
    logstream_ << error << std::endl;
    
    logstream_.close();
    log_mutex_.unlock();      

    return 0;
  };

  // User to override the default log file (/var/log/lab-daq/fast-daq.log).
  void SetLogFile(const std::string& logfile) {
    log_mutex_.lock();
    logfile_ = logfile;
    log_mutex_.unlock();      
  };

 private:

  enum level { DUMP, DEBUG, MESSAGE, WARNING, ERROR };

  inline void prepend(level lvl) {

    static timespec t;
    static tm_start;
    static lvl_msg;

    clock_gettime(CLOCK_REALTIME, &t);
    tm_start = std::put_time(localtime(&t.tv_sec), "%F %T.");

    switch (lvl) {
      case DUMP:
        lvl_msg = std::string(" ==RAW== ");
        break;
      case DEBUG:
        lvl_msg = std::string(" =DEBUG= ");
        break;
      case MESSAGE:
        lvl_msg = std::string(" MESSAGE ");
        break;
      case WARNING:
        lvl_msg = std::string("*WARNING*");
        break;
      case ERROR:
        lvl_msg = std::string("**ERROR**");
        break;
    }

    logstream_ << std::left << std::setw(time_width_) << tm_start;
    logstream_ << std::setfill('0') << std::setw(6) << t.tv_nsec / 1000; 
    logstream_ << lvl_msg;
  }
};

} // daq

#endif
