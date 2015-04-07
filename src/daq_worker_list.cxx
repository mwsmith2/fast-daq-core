#include "daq_worker_list.hh"
#include "daq_common_extdef.hh"

namespace daq {

void DaqWorkerList::StartRun()
{
  StartThreads();
  StartWorkers();
}

void DaqWorkerList::StopRun()
{
  StopWorkers();
  StopThreads();
}

void DaqWorkerList::StartWorkers()
{
  // Starts gathering data.
  if (logging_on) {
    logstream << "DaqWorkerList (*" << this << "): starting workers." << endl;
  }

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    // Needs a case switch to work with boost::variant vector.
    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartWorker();

    }
  }
}

void DaqWorkerList::StartThreads()
{
  // Launches the data worker threads.
  if (logging_on) {
    logstream << "DaqWorkerList (*" << this;
    logstream << "): launching worker threads." << endl;
  }

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartThread();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartThread();

    }
  }
}

void DaqWorkerList::StopWorkers()
{
  // Stop collecting data.
  if (logging_on) {
    logstream << "DaqWorkerList (*" << this << "): stopping workers." << endl;
  }

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopWorker();

    }
  }
}

void DaqWorkerList::StopThreads()
{
  // Stop and rejoin worker threads.
  if (logging_on) {
    logstream << "DaqWorkerList (*" << this;
    logstream << "): stopping worker threads." << endl;
  }
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopThread();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopThread();

    }
  }
}

bool DaqWorkerList::AllWorkersHaveEvent()
{
  // Check each worker for an event.
  bool has_event = true;
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    // If any worker doesn't have an event, has_event will become false.
    if ((*it).which() == 0) {

      has_event &= boost::get<DaqWorkerBase<sis_3350> *>(*it)->HasEvent();

    } else if ((*it).which() == 1) {

      has_event &= boost::get<DaqWorkerBase<sis_3302> *>(*it)->HasEvent();

    }
  }

  return has_event;
}

bool DaqWorkerList::AnyWorkersHaveEvent()
{
  // Check each worker for an event.
  bool any_events = false;
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    // A bitwise or here works, so that any event will return positive.
    if ((*it).which() == 0) {

      any_events |= boost::get<DaqWorkerBase<sis_3350> *>(*it)->HasEvent();

    } else if ((*it).which() == 1) {

      any_events |= boost::get<DaqWorkerBase<sis_3302> *>(*it)->HasEvent();

    }
  }

  return any_events;
}

bool DaqWorkerList::AnyWorkersHaveMultiEvent()
{
  // Check each worker for more than one event.
  int num_events = 0;
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      num_events = boost::get<DaqWorkerBase<sis_3350> *>(*it)->num_events();

    } else if ((*it).which() == 1) {

      num_events = boost::get<DaqWorkerBase<sis_3302> *>(*it)->num_events();

    }

    if (num_events > 1) return true;
  }

  return false;
}

void DaqWorkerList::GetEventData(vme_data &bundle)
{
  // Loops over each worker and collect the event data.
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      auto ptr = boost::get<DaqWorkerBase<sis_3350> *>(*it);
      bundle.sis_fast.push_back(ptr->PopEvent());

    } else if ((*it).which() == 1) {

      auto ptr = boost::get<DaqWorkerBase<sis_3302> *>(*it);
      bundle.sis_slow.push_back(ptr->PopEvent());

    }
  }
}

void DaqWorkerList::FlushEventData()
{
  // Drops any stale events when workers should have no events.
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->FlushEvents();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->FlushEvents();

    }
  } 
}

void DaqWorkerList::ClearList()
{
  // Remove the pointer references
  daq_workers_.resize(0);
}

void DaqWorkerList::FreeList()
{
  // Delete the allocated workers.
  if (logging_on) {
    logstream << "DaqWorkerList (*" << this;
    logstream << "): freeing workers." << endl;
  }
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {       

    if ((*it).which() == 0) {

      delete boost::get<DaqWorkerBase<sis_3350> *>(*it);

    } else if ((*it).which() == 1) {

      delete boost::get<DaqWorkerBase<sis_3302> *>(*it);

    }
  }

  if (vme_dev != -1) {
    close(vme_dev);
    vme_dev = -1;
  }
  
  daq_workers_.resize(0);
}

} // ::daq
