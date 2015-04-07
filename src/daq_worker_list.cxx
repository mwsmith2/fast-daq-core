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
  WriteLog("DaqWorkerList: starting workers.");

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    // Needs a case switch to work with boost::variant vector.
    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartWorker();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StartWorker();

    } else if ((*it).which() == 3) {

      boost::get<DaqWorkerBase<caen_6742> *>(*it)->StartWorker();

    } else if ((*it).which() == 4) {

      boost::get<DaqWorkerBase<drs4> *>(*it)->StartWorker();
    }
  }
}

void DaqWorkerList::StartThreads()
{
  // Launches the data worker threads.
  WriteLog("DaqWorkerList: launching worker threads.");

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartThread();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartThread();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StartThread();

    } else if ((*it).which() == 3) {

      boost::get<DaqWorkerBase<caen_6742> *>(*it)->StartThread();

    } else if ((*it).which() == 4) {

      boost::get<DaqWorkerBase<drs4> *>(*it)->StartThread();
    }
  }
}

void DaqWorkerList::StopWorkers()
{
  // Stop collecting data.
  WriteLog("DaqWorkerList: stopping workers.");

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopWorker();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StopWorker();

    } else if ((*it).which() == 3) {

      boost::get<DaqWorkerBase<caen_6742> *>(*it)->StopWorker();

    } else if ((*it).which() == 4) {

      boost::get<DaqWorkerBase<drs4> *>(*it)->StopWorker();
    }
  }
}

void DaqWorkerList::StopThreads()
{
  // Stop and rejoin worker threads.
  WriteLog("DaqWorkerList: stopping worker threads.");

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopThread();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopThread();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StopThread();

    } else if ((*it).which() == 3) {

      boost::get<DaqWorkerBase<caen_6742> *>(*it)->StopThread();

    } else if ((*it).which() == 4) {

      boost::get<DaqWorkerBase<drs4> *>(*it)->StopThread();
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

    } else if ((*it).which() == 2) {

      has_event &= boost::get<DaqWorkerBase<caen_1785> *>(*it)->HasEvent();

    } else if ((*it).which() == 3) {

      has_event &= boost::get<DaqWorkerBase<caen_6742> *>(*it)->HasEvent();

    } else if ((*it).which() == 4) {

      has_event &= boost::get<DaqWorkerBase<drs4> *>(*it)->HasEvent();
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

    } else if ((*it).which() == 2) {

      any_events |= boost::get<DaqWorkerBase<caen_1785> *>(*it)->HasEvent();

    } else if ((*it).which() == 3) {

      any_events |= boost::get<DaqWorkerBase<caen_6742> *>(*it)->HasEvent();

    } else if ((*it).which() == 4) {

      any_events |= boost::get<DaqWorkerBase<drs4> *>(*it)->HasEvent();
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

    } else if ((*it).which() == 2) {

      num_events = boost::get<DaqWorkerBase<caen_1785> *>(*it)->num_events();

    } else if ((*it).which() == 3) {

      num_events = boost::get<DaqWorkerBase<caen_6742> *>(*it)->num_events();

    } else if ((*it).which() == 4) {

      num_events = boost::get<DaqWorkerBase<drs4> *>(*it)->num_events();
    }

    if (num_events > 1) return true;
  }

  return false;
}

void DaqWorkerList::GetEventData(event_data &bundle)
{
  // Loops over each worker and collect the event data.
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      auto ptr = boost::get<DaqWorkerBase<sis_3350> *>(*it);
      bundle.sis_fast.push_back(ptr->PopEvent());

    } else if ((*it).which() == 1) {

      auto ptr = boost::get<DaqWorkerBase<sis_3302> *>(*it);
      bundle.sis_slow.push_back(ptr->PopEvent());

    } else if ((*it).which() == 2) {

      auto ptr = boost::get<DaqWorkerBase<caen_1785> *>(*it);
      bundle.caen_adc.push_back(ptr->PopEvent());

    } else if ((*it).which() == 3) {

      auto ptr = boost::get<DaqWorkerBase<caen_6742> *>(*it);
      bundle.caen_drs.push_back(ptr->PopEvent());

    } else if ((*it).which() == 4) {

      auto ptr = boost::get<DaqWorkerBase<drs4> *>(*it);
      bundle.drs.push_back(ptr->PopEvent());

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

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->FlushEvents();

    } else if ((*it).which() == 3) {

      boost::get<DaqWorkerBase<caen_6742> *>(*it)->FlushEvents();

    } else if ((*it).which() == 4) {

      boost::get<DaqWorkerBase<drs4> *>(*it)->FlushEvents();
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
  WriteLog("WorkerList: freeing workers.");

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {       

    if ((*it).which() == 0) {

      delete boost::get<DaqWorkerBase<sis_3350> *>(*it);

    } else if ((*it).which() == 1) {

      delete boost::get<DaqWorkerBase<sis_3302> *>(*it);

    } else if ((*it).which() == 2) {

      delete boost::get<DaqWorkerBase<caen_1785> *>(*it);

    } else if ((*it).which() == 3) {

      delete boost::get<DaqWorkerBase<caen_6742> *>(*it);

    } else if ((*it).which() == 4) {

      delete boost::get<DaqWorkerBase<drs4> *>(*it);
    }
  }

  if (vme_dev != -1) {
    close(vme_dev);
    vme_dev = -1;
  }
  
  daq_workers_.resize(0);
}

} // ::daq
