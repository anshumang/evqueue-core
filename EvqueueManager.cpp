/*
 * This file is part of evQueue
 * 
 * evQueue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * evQueue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with evQueue. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Anshuman Goswami <anshumang@gatech.edu>
 */

#include <EvqueueManager.h>

static uint64_t m_start_timestamp;
static uint32_t m_kernel_ctr, m_memcpy_h2d_ctr, m_memcpy_d2h_ctr, m_overhead_ctr;
static uint64_t m_kernel_window_start, m_kernel_window_end, m_kernel_cumul_occ, m_memcpy_h2d_cumul_occ, m_memcpy_d2h_cumul_occ, m_overhead_cumul_occ;

void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
  std::cerr << "bufferRequested" << std::endl;
  uint8_t *bfr = (uint8_t *) malloc(BUF_SIZE + ALIGN_SIZE);
  if (bfr == NULL) {
    printf("Error: out of memory\n");
    exit(-1);
  }

  *size = BUF_SIZE;
  *buffer = ALIGN_BUFFER(bfr, ALIGN_SIZE);
  *maxNumRecords = 0;
}

void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
  std::cerr << "bufferCompleted with " << validSize << " bytes of records"  << std::endl;
  CUptiResult status;
  CUpti_Activity *record = NULL;
  unsigned long long last_end=0;

  if (validSize > 0) {

    do {
      status = cuptiActivityGetNextRecord(buffer, validSize, &record);
      if (status == CUPTI_SUCCESS) {
        //printActivity(record);
        if((record->kind == CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)||(record->kind == CUPTI_ACTIVITY_KIND_KERNEL))
        {
          CUpti_ActivityKernel2 *kernel_record = (CUpti_ActivityKernel2 *) record;
          m_kernel_ctr++;
          if(kernel_record->start < m_kernel_window_start){
            m_kernel_window_start = kernel_record->start; //gets updated once
          } 
          if(kernel_record->end > m_kernel_window_end){
            m_kernel_window_end = kernel_record->end; //gets updated with each new record
          }
          /*valid only for single stream*/
          unsigned long long running = kernel_record->end - kernel_record->start;
          m_kernel_cumul_occ+=running; 
          if(running > 10e6) /*used for >10 ms*/
          {
            long_running_t tmp;
            tmp.grid = (kernel_record->gridX) * (kernel_record->gridY) * (kernel_record->gridZ);
            tmp.start = kernel_record->start;
            tmp.running = running;
          }
          unsigned long long idle=0;
          if(last_end>0)
          {
             idle = kernel_record->start - last_end;
             if(idle > 10e6) /*couldn't use for >10 ms*/
             {
               long_idle_t tmp;
               tmp.start = last_end;
               tmp.idle = idle;
             } 
          }
          else
          {
             last_end = kernel_record->end;
          }
        }
        else if(record->kind == CUPTI_ACTIVITY_KIND_MEMCPY)
        {
          CUpti_ActivityMemcpy2 *memcpy_record = (CUpti_ActivityMemcpy2 *) record;
          if(memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_HTOD)
          {
             m_memcpy_h2d_ctr++;
             /*valid only for single stream*/
             unsigned long long running = memcpy_record->end - memcpy_record->start;
             m_memcpy_h2d_cumul_occ+=running; 
	  }
          else if(memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_DTOH)
          {
             m_memcpy_d2h_ctr++;
             /*valid only for single stream*/
             unsigned long long running = memcpy_record->end - memcpy_record->start;
             m_memcpy_d2h_cumul_occ+=running; 
          }
          else
          {
             std::cerr<< "Unhandled memcpy record type" << std::endl;
          }
        }
        else if(record->kind == CUPTI_ACTIVITY_KIND_OVERHEAD)
        {
          CUpti_ActivityOverhead *overhead_record = (CUpti_ActivityOverhead *) record;
	  m_overhead_ctr++;
	  /*valid only for single stream*/
	  unsigned long long running = overhead_record->end - overhead_record->start;
	  m_overhead_cumul_occ+=running;
        }
        else
        {
           std::cerr<< "Unhandled record type" << std::endl;
        }
      }
      else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED)
        break;
      else {
        CUPTI_CALL(status);
      }
    } while (1);

    // report any records dropped from the queue
    size_t dropped;
    CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
    if (dropped != 0) {
      printf("Dropped %u activity records\n", (unsigned int) dropped);
    }

    std::cerr << 
    " # of kernels : " << m_kernel_ctr <<
    " Time window(in ns) : " << m_kernel_window_end - m_kernel_window_start <<
    " Used for(in ns) : " << m_kernel_cumul_occ <<
    std::endl;

    //reset counters
    m_kernel_ctr=m_memcpy_h2d_ctr=m_memcpy_d2h_ctr=m_overhead_ctr=0;
    m_kernel_cumul_occ=m_memcpy_h2d_cumul_occ=m_memcpy_d2h_cumul_occ=m_overhead_cumul_occ=0;
    m_kernel_window_start=ULLONG_MAX;
    m_kernel_window_end=0;
  }

  free(buffer);
}

EvqueueManager::EvqueueManager(void)
{
  size_t attrValue = 0, attrValueSize = sizeof(size_t);
  attrValue = 256*1024;
  CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
  attrValue = 1;
  CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));
  CUPTI_CALL(cuptiGetTimestamp(&m_start_timestamp));
  m_kernel_ctr=m_memcpy_h2d_ctr=m_memcpy_d2h_ctr=m_overhead_ctr=0;
  m_kernel_cumul_occ=m_memcpy_h2d_cumul_occ=m_memcpy_d2h_cumul_occ=m_overhead_cumul_occ=0;
  m_kernel_window_start=ULLONG_MAX;
  m_kernel_window_end=0;

     //m_evqueue_client = new EvqueueClient();
}

EvqueueManager::~EvqueueManager(void)
{
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));
     //delete m_evqueue_client;
}

void EvqueueManager::synch(void)
{
  std::cerr << "Synch" <<std::endl;
  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));
    //m_evqueue_client->synch();
}
