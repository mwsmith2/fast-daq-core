#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>
#include <algorithm>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a caen_1742 device.
namespace daq {

typedef struct {
  int16_t cell[CAEN_1742_CH][1024];
  int8_t  nsample[CAEN_1742_CH][1024];
  float   time[CAEN_1742_GR][1024];
} drs_correction;

class WorkerCaen1742 : public WorkerVme<caen_1742> {

 public:
  
  // ctor
  WorkerCaen1742(std::string name, std::string conf);

  // dtor
  ~WorkerCaen1742();

  int dev_sn() { return 0;};
  void LoadConfig();
  void WorkLoop();
  caen_1742 PopEvent();

private:

  const float vpp_ = 1.0;
  const ushort peakthresh = 30;
  
  int device_;
  uint sampling_setting_;
  uint size_, bsize_;
  char *buffer_;
  bool drs_cell_corrections_;
  bool drs_peak_corrections_;
  bool drs_time_corrections_;

  std::chrono::high_resolution_clock::time_point t0_;

  bool EventAvailable();
  void GetEvent(caen_1742 &bundle);

  int ApplyDataCorrection(caen_1742 &data, const std::vector<uint> &startcells);
  int PeakCorrection(caen_1742 &data, const drs_correction &table);
  int CellCorrection(caen_1742 &data, 
		     const drs_correction &table, 
		     const std::vector<uint> &startcells);
  int TimeCorrection(caen_1742 &data, 
		     const drs_correction &table, 
		     const std::vector<uint> &startcells);
  int GetCorrectionData(drs_correction &table);
  int GetChannelCorrectionData(uint ch, drs_correction &table);
  int ReadFlashPage(uint32_t group, uint32_t pagenum, std::vector<int8_t> &page);
  int WriteCorrectionDataCsv();
};

} // ::daq

#endif
