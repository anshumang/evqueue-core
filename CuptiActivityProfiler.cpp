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

#include <CuptiActivityProfiler.h>

CuptiActivityProfiler::CuptiActivityProfiler(void)
{
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  CUPTI_CALL(cuptiActivityRegisterCallbacks(reinterpret_cast<void (*)(unsigned char**, long unsigned int*, long unsigned int*)>(&CuptiActivityProfiler::bufferRequested), reinterpret_cast<void (*)(CUctx_st*, unsigned int, unsigned char*, long unsigned int, long unsigned int)>(&CuptiActivityProfiler::bufferCompleted)));
  CUPTI_CALL(cuptiGetTimestamp(&m_start_timestamp));
}

CuptiActivityProfiler::~CuptiActivityProfiler(void)
{
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));

}

const char *CuptiActivityProfiler::getMemcpyKindString(CUpti_ActivityMemcpyKind kind)
{
  switch (kind) {
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOD:
    return "HtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOH:
    return "DtoH";
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
    return "HtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
    return "AtoH";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
    return "AtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
    return "AtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
    return "DtoA";
  case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
    return "DtoD";
  case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
    return "HtoH";
  default:
    break;
  }

  return "<unknown>";
}

const char *CuptiActivityProfiler::getActivityOverheadKindString(CUpti_ActivityOverheadKind kind)
{
  switch (kind) {
  case CUPTI_ACTIVITY_OVERHEAD_DRIVER_COMPILER:
    return "COMPILER";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_BUFFER_FLUSH:
    return "BUFFER_FLUSH";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_INSTRUMENTATION:
    return "INSTRUMENTATION";
  case CUPTI_ACTIVITY_OVERHEAD_CUPTI_RESOURCE:
    return "RESOURCE";
  default:
    break;
  }

  return "<unknown>";
}

void CUPTIAPI CuptiActivityProfiler::bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
  uint8_t *bfr = (uint8_t *) malloc(BUF_SIZE + ALIGN_SIZE);
  if (bfr == NULL) {
    printf("Error: out of memory\n");
    exit(-1);
  }

  *size = BUF_SIZE;
  *buffer = ALIGN_BUFFER(bfr, ALIGN_SIZE);
  *maxNumRecords = 0;
}

void CUPTIAPI CuptiActivityProfiler::bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
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
          CUpti_ActivityMemcpy2 *memcpy_record = (CUpti_ActivityMemcpy *) record;
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

  }

  free(buffer);
}

