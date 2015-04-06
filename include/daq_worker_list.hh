#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_LIST_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_LIST_HH_

//--- std includes ----------------------------------------------------------//
#include <vector>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>

//--- project includes ------------------------------------------------------//
#include "daq_structs.hh"
#include "daq_worker_fake.hh"
#include "daq_worker_sis3350.hh"
#include "daq_worker_sis3302.hh"
#include "daq_worker_caen1785.hh"
#include "daq_worker_caen6742.hh"
#include "daq_worker_drs4.hh"

namespace daq {

class DaqWorkerList {

  public:

    // ctor
    DaqWorkerList();

    // dtor
    ~DaqWorkerList();

    // Functions to control the runs.
    void StartRun();
    void StopRun();

    void StartThreads();
    void StopThreads();
    void StartWorkers();
    void StopWorkers();
    bool AllWorkersHaveEvent();
    bool AnyWorkersHaveEvent();
    bool AnyWorkersHaveMultiEvent();
    void GetEventData(event_data &bundle);
    void FlushEventData();

    // Functions to structure the list
    void PushBack(worker_ptr_types daq_worker) {
      daq_workers_.push_back(daq_worker);
    }

    void FreeList();

  private:

    std::vector<worker_ptr_types> daq_workers_;
};

} // ::daq

#endif
