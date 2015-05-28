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

#ifndef _CUPTI_ACTIVITY_PROFILER_H_
#define _CUPTI_ACTIVITY_PROFILER_H_

#include <cstdio>
#include <climits>
#include <iostream>
#include <cupti.h>
#include <nn.h>
#include <pipeline.h>

#define CUPTI_CALL(call)                                                \
  do {                                                                  \
    CUptiResult _status = call;                                         \
    if (_status != CUPTI_SUCCESS) {                                     \
      const char *errstr;                                               \
      cuptiGetResultString(_status, &errstr);                           \
      fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
              __FILE__, __LINE__, #call, errstr);                       \
      exit(-1);                                                         \
    }                                                                   \
  } while (0)

#define BUF_SIZE (32 * 1024 * 1024)
//#define BUF_SIZE (112)
#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
  (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))

//extern "C" void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize);
//extern "C" void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords);

class CuptiActivityProfiler
{
     public:
          CuptiActivityProfiler(void);
          ~CuptiActivityProfiler(void);
          void synch(void);
          //void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords);
          //void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize);
     private:
          uint64_t m_start_timestamp;
          uint32_t m_kernel_ctr, m_memcpy_h2d_ctr, m_memcpy_d2h_ctr, m_overhead_ctr;
          uint64_t m_kernel_window_start, m_kernel_window_end, m_kernel_cumul_occ, m_memcpy_h2d_cumul_occ, m_memcpy_d2h_cumul_occ, m_overhead_cumul_occ;
};

#endif
