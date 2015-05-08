#include "worker_list.hh"
#include "common_extdef.hh"

namespace daq {

void WorkerList::StartRun()
{
  StartThreads();
  StartWorkers();
}

void WorkerList::StopRun()
{
  StopWorkers();
  StopThreads();
}

void WorkerList::StartWorkers()
{
  // Starts gathering data.
  WriteLog("WorkerList: starting workers.");

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    // Needs a case switch to work with boost::variant vector.
    if ((*it).which() == 0) {

      boost::get<WorkerBase<sis_3350> *>(*it)->StartWorker();

    } else if ((*it).which() == 1) {

      boost::get<WorkerBase<sis_3302> *>(*it)->StartWorker();

    } else if ((*it).which() == 2) {

      boost::get<WorkerBase<caen_1785> *>(*it)->StartWorker();

    } else if ((*it).which() == 3) {

      boost::get<WorkerBase<caen_6742> *>(*it)->StartWorker();

    } else if ((*it).which() == 4) {

      boost::get<WorkerBase<drs4> *>(*it)->StartWorker();

    } else if ((*it).which() == 5) {

      boost::get<WorkerBase<caen_1742> *>(*it)->StartWorker();
    }
  }
}

void WorkerList::StartThreads()
{
  // Launches the data worker threads.
  WriteLog("WorkerList: launching worker threads.");

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<WorkerBase<sis_3350> *>(*it)->StartThread();

    } else if ((*it).which() == 1) {

      boost::get<WorkerBase<sis_3302> *>(*it)->StartThread();

    } else if ((*it).which() == 2) {

      boost::get<WorkerBase<caen_1785> *>(*it)->StartThread();

    } else if ((*it).which() == 3) {

      boost::get<WorkerBase<caen_6742> *>(*it)->StartThread();

    } else if ((*it).which() == 4) {

      boost::get<WorkerBase<drs4> *>(*it)->StartThread();

    } else if ((*it).which() == 5) {

      boost::get<WorkerBase<caen_1742> *>(*it)->StartThread();
    }
  }
}

void WorkerList::StopWorkers()
{
  // Stop collecting data.
  WriteLog("WorkerList: stopping workers.");

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<WorkerBase<sis_3350> *>(*it)->StopWorker();

    } else if ((*it).which() == 1) {

      boost::get<WorkerBase<sis_3302> *>(*it)->StopWorker();

    } else if ((*it).which() == 2) {

      boost::get<WorkerBase<caen_1785> *>(*it)->StopWorker();

    } else if ((*it).which() == 3) {

      boost::get<WorkerBase<caen_6742> *>(*it)->StopWorker();

    } else if ((*it).which() == 4) {

      boost::get<WorkerBase<drs4> *>(*it)->StopWorker();

    } else if ((*it).which() == 5) {

      boost::get<WorkerBase<caen_1742> *>(*it)->StopWorker();
    }
  }
}

void WorkerList::StopThreads()
{
  // Stop and rejoin worker threads.
  WriteLog("WorkerList: stopping worker threads.");

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<WorkerBase<sis_3350> *>(*it)->StopThread();

    } else if ((*it).which() == 1) {

      boost::get<WorkerBase<sis_3302> *>(*it)->StopThread();

    } else if ((*it).which() == 2) {

      boost::get<WorkerBase<caen_1785> *>(*it)->StopThread();

    } else if ((*it).which() == 3) {

      boost::get<WorkerBase<caen_6742> *>(*it)->StopThread();

    } else if ((*it).which() == 4) {

      boost::get<WorkerBase<drs4> *>(*it)->StopThread();

    } else if ((*it).which() == 5) {

      boost::get<WorkerBase<caen_1742> *>(*it)->StopThread();
    }
  }
}

bool WorkerList::AllWorkersHaveEvent()
{
  // Check each worker for an event.
  bool has_event = true;

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    // If any worker doesn't have an event, has_event will become false.
    if ((*it).which() == 0) {

      has_event &= boost::get<WorkerBase<sis_3350> *>(*it)->HasEvent();

    } else if ((*it).which() == 1) {

      has_event &= boost::get<WorkerBase<sis_3302> *>(*it)->HasEvent();

    } else if ((*it).which() == 2) {

      has_event &= boost::get<WorkerBase<caen_1785> *>(*it)->HasEvent();

    } else if ((*it).which() == 3) {

      has_event &= boost::get<WorkerBase<caen_6742> *>(*it)->HasEvent();

    } else if ((*it).which() == 4) {

      has_event &= boost::get<WorkerBase<drs4> *>(*it)->HasEvent();

    } else if ((*it).which() == 5) {

      has_event &= boost::get<WorkerBase<caen_1742> *>(*it)->HasEvent();
    }
  }

  return has_event;
}

bool WorkerList::AnyWorkersHaveEvent()
{
  // Check each worker for an event.
  bool any_events = false;

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    // A bitwise or here works, so that any event will return positive.
    if ((*it).which() == 0) {

      any_events |= boost::get<WorkerBase<sis_3350> *>(*it)->HasEvent();

    } else if ((*it).which() == 1) {

      any_events |= boost::get<WorkerBase<sis_3302> *>(*it)->HasEvent();

    } else if ((*it).which() == 2) {

      any_events |= boost::get<WorkerBase<caen_1785> *>(*it)->HasEvent();

    } else if ((*it).which() == 3) {

      any_events |= boost::get<WorkerBase<caen_6742> *>(*it)->HasEvent();

    } else if ((*it).which() == 4) {

      any_events |= boost::get<WorkerBase<drs4> *>(*it)->HasEvent();

    } else if ((*it).which() == 5) {

      any_events |= boost::get<WorkerBase<caen_1742> *>(*it)->HasEvent();
    }
  }

  return any_events;
}

bool WorkerList::AnyWorkersHaveMultiEvent()
{
  // Check each worker for more than one event.
  int num_events = 0;
  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      num_events = boost::get<WorkerBase<sis_3350> *>(*it)->num_events();

    } else if ((*it).which() == 1) {

      num_events = boost::get<WorkerBase<sis_3302> *>(*it)->num_events();

    } else if ((*it).which() == 2) {

      num_events = boost::get<WorkerBase<caen_1785> *>(*it)->num_events();

    } else if ((*it).which() == 3) {

      num_events = boost::get<WorkerBase<caen_6742> *>(*it)->num_events();

    } else if ((*it).which() == 4) {

      num_events = boost::get<WorkerBase<drs4> *>(*it)->num_events();

    } else if ((*it).which() == 5) {

      num_events = boost::get<WorkerBase<caen_1742> *>(*it)->num_events();
    }

    if (num_events > 1) return true;
  }

  return false;
}

void WorkerList::GetEventData(event_data &bundle)
{
  // Loops over each worker and collect the event data.
  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      auto ptr = boost::get<WorkerBase<sis_3350> *>(*it);
      bundle.sis_fast.push_back(ptr->PopEvent());

    } else if ((*it).which() == 1) {

      auto ptr = boost::get<WorkerBase<sis_3302> *>(*it);
      bundle.sis_slow.push_back(ptr->PopEvent());

    } else if ((*it).which() == 2) {

      auto ptr = boost::get<WorkerBase<caen_1785> *>(*it);
      bundle.caen_adc.push_back(ptr->PopEvent());

    } else if ((*it).which() == 3) {

      auto ptr = boost::get<WorkerBase<caen_6742> *>(*it);
      bundle.caen_drs.push_back(ptr->PopEvent());

    } else if ((*it).which() == 4) {

      auto ptr = boost::get<WorkerBase<drs4> *>(*it);
      bundle.drs.push_back(ptr->PopEvent());

    } else if ((*it).which() == 5) {

      auto ptr = boost::get<WorkerBase<caen_1742> *>(*it);
      bundle.caen_1742_vec.push_back(ptr->PopEvent());
    }
  }
}

void WorkerList::FlushEventData()
{
  // Drops any stale events when workers should have no events.
  for (auto it = workers_.begin(); it != workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<WorkerBase<sis_3350> *>(*it)->FlushEvents();

    } else if ((*it).which() == 1) {

      boost::get<WorkerBase<sis_3302> *>(*it)->FlushEvents();

    } else if ((*it).which() == 2) {

      boost::get<WorkerBase<caen_1785> *>(*it)->FlushEvents();

    } else if ((*it).which() == 3) {

      boost::get<WorkerBase<caen_6742> *>(*it)->FlushEvents();

    } else if ((*it).which() == 4) {

      boost::get<WorkerBase<drs4> *>(*it)->FlushEvents();

    } else if ((*it).which() == 5) {

      boost::get<WorkerBase<caen_1742> *>(*it)->FlushEvents();
    }
  } 
}

void WorkerList::FreeList()
{
  // Delete the allocated workers.
  WriteLog("WorkerList: freeing workers.");

  for (auto it = workers_.begin(); it != workers_.end(); ++it) {       

    if ((*it).which() == 0) {

      delete boost::get<WorkerBase<sis_3350> *>(*it);

    } else if ((*it).which() == 1) {

      delete boost::get<WorkerBase<sis_3302> *>(*it);

    } else if ((*it).which() == 2) {

      delete boost::get<WorkerBase<caen_1785> *>(*it);

    } else if ((*it).which() == 3) {

      delete boost::get<WorkerBase<caen_6742> *>(*it);

    } else if ((*it).which() == 4) {

      delete boost::get<WorkerBase<drs4> *>(*it);

    } else if ((*it).which() == 5) {

      delete boost::get<WorkerBase<caen_1742> *>(*it);
    }
  }

  Resize(0);
}

} // ::daq
