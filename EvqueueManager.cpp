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
#include <Messages.h>
#include <nn.h>
#include <pipeline.h>

static uint64_t m_start_timestamp;
static uint32_t m_kernel_ctr, m_memcpy_h2d_ctr, m_memcpy_d2h_ctr, m_overhead_ctr;
static uint64_t m_kernel_window_start, m_kernel_window_end, m_kernel_cumul_occ, m_memcpy_h2d_window_start, m_memcpy_h2d_window_end, m_memcpy_h2d_cumul_occ, m_memcpy_d2h_window_start, m_memcpy_d2h_window_end, m_memcpy_d2h_cumul_occ, m_overhead_window_start, m_overhead_window_end, m_overhead_cumul_occ;
static bool done=false;
static uint64_t m_last_kernel_end, m_last_api_end, m_last_memcpy_end, m_last_memset_end, m_last_overhead_end;
static uint64_t m_last_kernel_grid[3], m_last_kernel_block[3];

int m_sock;
const char *m_url;

int numCompletions;

void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
  //std::cerr << "bufferRequested" << std::endl;
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
  //if(!done)
  //{
    //CUPTI_CALL(cuptiGetTimestamp(&m_start_timestamp));
    //std::cerr << "Starting at " << m_start_timestamp << std::endl;
    //done = true;
  //}
  //std::cerr << "bufferCompleted with " << validSize << " bytes of records (stream " << streamId << ")"  << std::endl;
  CUptiResult status;
  CUpti_Activity *record = NULL;

  if (validSize > 0) {

  int numRecords = 0, numKernelRecords = 0;
  numCompletions++;

  ClientMessage *msg = new ClientMessage;

    do {
      status = cuptiActivityGetNextRecord(buffer, validSize, &record);
      if (status == CUPTI_SUCCESS) {
        numRecords++;
        if((record->kind == CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL)||(record->kind == CUPTI_ACTIVITY_KIND_KERNEL))
        {
          numKernelRecords++;
          CUpti_ActivityKernel2 *kernel_record = (CUpti_ActivityKernel2 *) record;
          //m_kernel_ctr++;
          //if(kernel_record->start - m_start_timestamp < m_kernel_window_start){
            //m_kernel_window_start = kernel_record->start - m_start_timestamp; //gets updated once
          //} 
          //if(kernel_record->end - m_start_timestamp > m_kernel_window_end){
            //m_kernel_window_end = kernel_record->end - m_start_timestamp; //gets updated with each new record
          //}
          if(m_last_kernel_end > 0)
          {
             //std::cerr << "IDLE-K " << m_last_kernel_end << " " << kernel_record->start - m_last_kernel_end - m_start_timestamp << std::endl;
             /*Gap > 10ms*/
             if(kernel_record->start - m_last_kernel_end - m_start_timestamp > 10000000)
             {
                //std::cout << "Long Gap : " << kernel_record->start - m_last_kernel_end - m_start_timestamp << std::endl;
                LongGap gap;
                gap.trailing_grid[0]=kernel_record->gridX;
                gap.trailing_grid[1]=kernel_record->gridY;
                gap.trailing_grid[2]=kernel_record->gridZ;
                gap.trailing_block[0]=kernel_record->blockX;
                gap.trailing_block[1]=kernel_record->blockY;
                gap.trailing_block[2]=kernel_record->blockZ;
                gap.duration=kernel_record->start-m_last_kernel_end-m_start_timestamp;
                msg->m_long_gaps.push_back(gap);
             }
             m_last_kernel_end = kernel_record->end - m_start_timestamp;
             m_last_kernel_grid[0]=kernel_record->gridX;
             m_last_kernel_grid[1]=kernel_record->gridY;
             m_last_kernel_grid[2]=kernel_record->gridZ;
             m_last_kernel_block[0]=kernel_record->blockX;
             m_last_kernel_block[1]=kernel_record->blockY;
             m_last_kernel_block[2]=kernel_record->blockZ;
          }
          else
          {
             m_last_kernel_end = kernel_record->end - m_start_timestamp;
          }
	  /*Use > 10ms*/
          if(kernel_record->end - kernel_record->start > 10000000)
          {
            //std::cout << "Long Use : " << kernel_record->end - kernel_record->start << std::endl;
            LongKernel use;
            msg->m_long_kernels.push_back(use);
          }
         //std::cout << "K" << kernel_record->correlationId << " " << kernel_record->start - m_start_timestamp << " " << kernel_record->end - kernel_record->start << std::endl;
#if 0
          /*valid only for single stream*/
          unsigned long long running = kernel_record->end - kernel_record->start;
          m_kernel_cumul_occ+=running; 
          unsigned long long idle=0;
          if(last_end>0)
          {
             idle = kernel_record->start - last_end;
             last_end = kernel_record->end;
          }
          else
          {
             last_end = kernel_record->end;
          }
#endif
        }
        else if(record->kind == CUPTI_ACTIVITY_KIND_MEMSET)
        {
          CUpti_ActivityMemset *memset_record = (CUpti_ActivityMemset *) record;
          if(m_last_memset_end > 0)
          {
             //std::cerr << "IDLE-MS " << m_last_memset_end << " " << memset_record->start - m_last_memset_end - m_start_timestamp << std::endl;
             m_last_memset_end = memset_record->end - m_start_timestamp;
          }
          else
          {
             m_last_memset_end = memset_record->end - m_start_timestamp;
          }
         //std::cerr << "MS" << memset_record->correlationId << " " << memset_record->start - m_start_timestamp << " " << memset_record->end - memset_record->start << " " << memset_record->runtimeCorrelationId << std::endl;

        }
        else if((record->kind == CUPTI_ACTIVITY_KIND_RUNTIME)||(record->kind == CUPTI_ACTIVITY_KIND_DRIVER))
        {
          CUpti_ActivityAPI *api_record = (CUpti_ActivityAPI *) record;
          if(m_last_api_end > 0)
          {
             //std::cerr << "IDLE-A " << m_last_api_end << " " << api_record->start - m_last_api_end - m_start_timestamp << std::endl;
             m_last_api_end = api_record->end - m_start_timestamp;
          }
          else
          {
             m_last_api_end = api_record->end - m_start_timestamp;
          }
         //std::cerr << "A" << api_record->correlationId << " " << api_record->start - m_start_timestamp << " " << api_record->end - api_record->start << std::endl;

        }
        else if(record->kind == CUPTI_ACTIVITY_KIND_MEMCPY)
        {
          CUpti_ActivityMemcpy *memcpy_record = (CUpti_ActivityMemcpy *) record;
          if(m_last_memcpy_end > 0)
          {
             //std::cerr << "IDLE-MC " << m_last_memcpy_end << " " << memcpy_record->start - m_last_memcpy_end - m_start_timestamp << std::endl;
             m_last_memcpy_end = memcpy_record->end - m_start_timestamp;
          }
          else
          {
             m_last_memcpy_end = memcpy_record->end - m_start_timestamp;
          }
          if(memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_HTOD)
          {
              //std::cerr << "MCH" << memcpy_record->runtimeCorrelationId << " " << memcpy_record->start - m_start_timestamp << " " << memcpy_record->end - memcpy_record->start << std::endl;
             //m_memcpy_h2d_ctr++;
	     //if(memcpy_record->start - m_start_timestamp < m_memcpy_h2d_window_start){
		     //m_memcpy_h2d_window_start = memcpy_record->start - m_start_timestamp; //gets updated once
	     //} 
	     //if(memcpy_record->end - m_start_timestamp > m_memcpy_h2d_window_end){
		     //m_memcpy_h2d_window_end = memcpy_record->end - m_start_timestamp; //gets updated with each new record
	     //}
             /*valid only for single stream*/
             //unsigned long long running = memcpy_record->end - memcpy_record->start;
             //m_memcpy_h2d_cumul_occ+=running; 
	  }
          else if(memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_DTOH)
          {
              //std::cerr << "MCD" << memcpy_record->runtimeCorrelationId << " " << memcpy_record->start - m_start_timestamp << " " << memcpy_record->end - memcpy_record->start << std::endl;
             //m_memcpy_d2h_ctr++;
	     //if(memcpy_record->start - m_start_timestamp < m_memcpy_d2h_window_start){
		     //m_memcpy_d2h_window_start = memcpy_record->start - m_start_timestamp; //gets updated once
	     //} 
	     //if(memcpy_record->end - m_start_timestamp > m_memcpy_d2h_window_end){
		     //m_memcpy_d2h_window_end = memcpy_record->end - m_start_timestamp; //gets updated with each new record
	     //}
             /*valid only for single stream*/
             //unsigned long long running = memcpy_record->end - memcpy_record->start;
             //m_memcpy_d2h_cumul_occ+=running; 
          }
          else
          {
             switch(memcpy_record->copyKind)
             {
               case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
                   std::cerr << "HtoA unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
                   std::cerr << "AtoH unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
                   std::cerr << "AtoA unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
                   std::cerr << "AtoD unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
                   std::cerr << "DtoA unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
                   std::cerr << "DtoD unhandled" << std::endl;
               case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
                   std::cerr << "HtoH unhandled" << std::endl;
               default:
                   std::cerr << "Other unhandled" << std::endl;
             }
          }
        }
        else if(record->kind == CUPTI_ACTIVITY_KIND_OVERHEAD)
        {
          CUpti_ActivityOverhead *overhead_record = (CUpti_ActivityOverhead *) record;
          if(m_last_overhead_end > 0)
          {
             //std::cerr << "IDLE-O " << m_last_overhead_end << " " << overhead_record->start - m_last_overhead_end - m_start_timestamp << std::endl;
             m_last_overhead_end = overhead_record->end - m_start_timestamp;
          }
          else
          {
             m_last_overhead_end = overhead_record->end - m_start_timestamp;
          }
         //std::cerr << "OVERHEAD " << overhead_record->start - m_start_timestamp << " " << overhead_record->end - overhead_record->start << std::endl;
          //if(overhead_record->start - m_start_timestamp < m_overhead_window_start){
            //m_overhead_window_start = overhead_record->start - m_start_timestamp; //gets updated once
          //} 
          //if(overhead_record->end - m_start_timestamp > m_overhead_window_end){
            //m_overhead_window_end = overhead_record->end - m_start_timestamp; //gets updated with each new record
          //}
	  //m_overhead_ctr++;
	  /*valid only for single stream*/
	  //unsigned long long running = overhead_record->end - overhead_record->start;
	  //m_overhead_cumul_occ+=running;
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

    std::cout << numCompletions << "/" << numRecords << 
    "/" << numKernelRecords << 
    "," << msg->m_long_gaps.size() << 
    "," << msg->m_long_kernels.size() << std::endl;

    // report any records dropped from the queue
    size_t dropped;
    CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
    if (dropped != 0) {
      printf("Dropped %u activity records\n", (unsigned int) dropped);
    }
    save_schedule(*msg);
    std::cout << "Sent bytes : " << msg->m_stream.tellp() << "/" << sizeof(CUpti_ActivityKernel2)*numKernelRecords << std::endl;
    //int bytes = nn_send (m_sock, msg, sizeof(ClientMessage), 0);
    //assert (bytes == sizeof(ClientMessage));

    //std::cerr << m_kernel_window_start - m_start_timestamp << " " << m_memcpy_d2h_window_start - m_start_timestamp << " " << m_overhead_window_start - m_start_timestamp <<
    //std::endl <<
    //std::cerr << m_kernel_window_end - m_start_timestamp << " " << m_memcpy_d2h_window_end - m_start_timestamp << " " << m_overhead_window_end - m_start_timestamp <<
    //std::endl <<

    //std::cerr << 
    //" # of kernels : " << m_kernel_ctr <<
    //" Time window(in ns) : " << m_kernel_window_end - m_kernel_window_start <<
    //" Used for(in ns) : " << m_kernel_cumul_occ <<
    //std::endl <<
    //" # of memcpys (H2D) : " << m_memcpy_h2d_ctr <<
    //" Used for(in ns) : " << m_memcpy_h2d_cumul_occ <<
    //std::endl <<
    //" # of memcpys (D2H) : " << m_memcpy_d2h_ctr <<
    //" Used for(in ns) : " << m_memcpy_d2h_cumul_occ <<
    //std::endl <<
    //" # of overheads : " << m_overhead_ctr <<
    //" Used for(in ns) : " << m_overhead_cumul_occ <<
    //std::endl;

    //reset counters
    /*m_kernel_ctr=m_memcpy_h2d_ctr=m_memcpy_d2h_ctr=m_overhead_ctr=0;
    m_kernel_cumul_occ=m_memcpy_h2d_cumul_occ=m_memcpy_d2h_cumul_occ=m_overhead_cumul_occ=0;
    m_kernel_window_start=m_memcpy_h2d_window_start=m_memcpy_d2h_window_start=m_overhead_window_start=ULLONG_MAX;
    m_kernel_window_end=m_memcpy_h2d_window_end=m_memcpy_d2h_window_end=m_overhead_window_end=0;
    */
  }

  free(buffer);
}

EvqueueManager::EvqueueManager(void)
{
  //Sending CUPTI profiling info
  m_url = "ipc:///tmp/pipeline.ipc";
  m_sock = nn_socket (AF_SP, NN_PUSH);
  assert (m_sock >= 0);
  assert (nn_connect (m_sock, m_url) >= 0);

  size_t attrValue = 0, attrValueSize = sizeof(size_t);
  attrValue = 256 * 1024;
  CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
  attrValue = 1;
  CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));

  /*CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  */
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  /*CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_NAME));
  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MARKER));
  */

  CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));
  CUPTI_CALL(cuptiGetTimestamp(&m_start_timestamp));
  /*m_kernel_ctr=m_memcpy_h2d_ctr=m_memcpy_d2h_ctr=m_overhead_ctr=0;
  m_kernel_cumul_occ=m_memcpy_h2d_cumul_occ=m_memcpy_d2h_cumul_occ=m_overhead_cumul_occ=0;
  m_kernel_window_start=m_memcpy_h2d_window_start=m_memcpy_d2h_window_start=m_overhead_window_start=ULLONG_MAX;
  m_kernel_window_end=m_memcpy_h2d_window_end=m_memcpy_d2h_window_end=m_overhead_window_end=0;
  */

     //m_evqueue_client = new EvqueueClient();
}

EvqueueManager::~EvqueueManager(void)
{
  nn_shutdown (m_sock, 0);
  /*CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_RUNTIME));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_DRIVER));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMSET));
  */
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
  /*CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_DEVICE));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONTEXT));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_NAME));
  CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MARKER));
  */


  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));
     //delete m_evqueue_client;
}

void EvqueueManager::synch(void)
{
  std::cerr << "Synch" <<std::endl;
  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));
    //m_evqueue_client->synch();
}
