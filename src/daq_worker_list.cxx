#include "daq_worker_list.hh"

// Include the extern defs here since it is always used.
#include "daq_structs_extdef.hh"

namespace daq {

DaqWorkerList::DaqWorkerList()
{
  // stub  
}

DaqWorkerList::~DaqWorkerList()
{
}

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
  // Start the data gatherers
  cout << "Starting workers." << endl;
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

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
  // Start the data gatherers
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
  // Stop the data gathering
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
  // Stop the data gathering
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

    if ((*it).which() == 0) {

      has_event &= boost::get<DaqWorkerBase<sis_3350> *>(*it)->HasEvent();
      if (0 & boost::get<DaqWorkerBase<sis_3350> *>(*it)->HasEvent()) {
	cout << boost::get<DaqWorkerBase<sis_3350> *>(*it)->name();
	cout << " has " << boost::get<DaqWorkerBase<sis_3350> *>(*it)->num_events() << " event." << endl;
      }

    } else if ((*it).which() == 1) {

      has_event &= boost::get<DaqWorkerBase<sis_3302> *>(*it)->HasEvent();
      if (0 & boost::get<DaqWorkerBase<sis_3302> *>(*it)->HasEvent()) {
	cout << boost::get<DaqWorkerBase<sis_3302> *>(*it)->name();
	cout << " has " << boost::get<DaqWorkerBase<sis_3302> *>(*it)->num_events() << " event." << endl;
      }

    } else if ((*it).which() == 2) {

      has_event &= boost::get<DaqWorkerBase<caen_1785> *>(*it)->HasEvent();
      if (0 & boost::get<DaqWorkerBase<caen_1785> *>(*it)->HasEvent()) {
	cout << boost::get<DaqWorkerBase<caen_1785> *>(*it)->name();
	cout << " has " << boost::get<DaqWorkerBase<caen_1785> *>(*it)->num_events() << " event." << endl;
      }

    } else if ((*it).which() == 3) {

      has_event &= boost::get<DaqWorkerBase<caen_6742> *>(*it)->HasEvent();
      if (0 & boost::get<DaqWorkerBase<caen_6742> *>(*it)->HasEvent()) {
	cout << boost::get<DaqWorkerBase<caen_6742> *>(*it)->name();
	cout << " has " << boost::get<DaqWorkerBase<caen_6742> *>(*it)->num_events() << " event." << endl;
      }

    } else if ((*it).which() == 4) {

      has_event &= boost::get<DaqWorkerBase<drs4> *>(*it)->HasEvent();
      if (0 & boost::get<DaqWorkerBase<drs4> *>(*it)->HasEvent()) {
        cout << boost::get<DaqWorkerBase<drs4> *>(*it)->name();
        cout << " has " << boost::get<DaqWorkerBase<drs4> *>(*it)->num_events() << " event." << endl;
      }
    }
  }

  return has_event;
}

bool DaqWorkerList::AnyWorkersHaveEvent()
{
  // Check each worker for an event.
  bool bad_data = false;
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      bad_data |= boost::get<DaqWorkerBase<sis_3350> *>(*it)->HasEvent();

    } else if ((*it).which() == 1) {

      bad_data |= boost::get<DaqWorkerBase<sis_3302> *>(*it)->HasEvent();

    } else if ((*it).which() == 2) {

      bad_data |= boost::get<DaqWorkerBase<caen_1785> *>(*it)->HasEvent();

    } else if ((*it).which() == 3) {

      bad_data |= boost::get<DaqWorkerBase<caen_6742> *>(*it)->HasEvent();

    } else if ((*it).which() == 4) {

      bad_data |= boost::get<DaqWorkerBase<drs4> *>(*it)->HasEvent();

    }

    if (bad_data == true) return bad_data;
  }

  return bad_data;
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

void DaqWorkerList::FreeList()
{
  // Delete the allocated workers.
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
