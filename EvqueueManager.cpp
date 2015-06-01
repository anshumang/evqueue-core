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

#include "EvqueueManager.h"
#include "Messages.h"

static uint64_t m_start_timestamp;
static uint64_t m_last_kernel_end, m_last_kernel_end_no_offset, m_last_api_end, m_last_memset_end, m_last_memcpy_end, m_last_overhead_end;
static uint64_t m_last_kernel_grid[3], m_last_kernel_block[3];

int numCompletions;

EvqueueManager *gEvqm = NULL;
int mSock;

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
          if(m_last_kernel_end > 0)
          {
             /*Gap > 10ms*/
             if((kernel_record->start - m_start_timestamp> m_last_kernel_end)&&(kernel_record->start - m_last_kernel_end - m_start_timestamp > 10000000))
             {
                //std::cout << "Gap " << kernel_record->start - m_last_kernel_end - m_start_timestamp << std::endl;
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
             m_last_kernel_end_no_offset = kernel_record->end;
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
             m_last_kernel_end_no_offset = kernel_record->end;
          }
	  /*Use > 10ms*/
          if(kernel_record->end - kernel_record->start > 10000000)
          {
            std::cout << kernel_record->end - kernel_record->start << " " << kernel_record->name << " " << kernel_record->gridX << " " << kernel_record->gridY << " " << kernel_record->gridZ << " " << kernel_record->blockX << " " << kernel_record->blockY << " " << kernel_record->blockZ << " " << std::endl;
            LongKernel use;
            msg->m_long_kernels.push_back(use);
          }
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
          if((memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_DTOH)||(memcpy_record->copyKind==CUPTI_ACTIVITY_MEMCPY_KIND_HTOD))
          {

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
    std::cout << "Sent bytes : " << msg->m_stream.tellp() << "/"
  << sizeof(ClientMessage) << "/" << sizeof(CUpti_ActivityKernel2)*numKernelRecords << std::endl;
   string ossbuf(msg->m_stream.str());
   gEvqm->mComm->send(reinterpret_cast<const void *>(ossbuf.c_str()), msg->m_stream.tellp());
   //int bytes = nn_send(mSock, (void *)(ossbuf.c_str()), msg->m_stream.tellp(), 0);
   //assert(bytes >= 0);
  }

  free(buffer);
}

EvqueueManager::EvqueueManager(void)
{
  std::cout << "EvqueueManager CTOR" << std::endl;
  mIposer = new Interposer();
  //Sending CUPTI profiling info
  std::string url("ipc:///tmp/pipeline.ipc");
  Component who=CLIENT;
  mComm = new Communicator(url, who);
  //assert((mSock = nn_socket(AF_SP, NN_PUSH)) >= 0);
  mComm->connect();
  //assert(nn_connect (mSock, url.c_str()) >= 0);

  size_t attrValue = 0, attrValueSize = sizeof(size_t);
  attrValue = 32 * 1024 * 1024;
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

}

EvqueueManager::~EvqueueManager(void)
{
  delete mComm;
  delete mIposer;
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
}

void EvqueueManager::synch(void)
{
  CUPTI_CALL(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_NONE));
}

int EvqueueManager::launch(KernelIdentifier kid)
{
  return mIposer->launch(kid);
}

extern "C"
void EvqueueCreate()
{
    gEvqm = new EvqueueManager();
}

extern "C"
void EvqueueDestroy()
{
    delete gEvqm;
}

extern "C"
void EvqueueLaunch(KernelIdentifier kid)
{
    gEvqm->launch(kid);
}

extern "C"
void EvqueueSynch()
{
    gEvqm->synch();
}
