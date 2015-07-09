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

extern uint64_t m_start_timestamp;
EvqueueManager *gEvqm = NULL;

EvqueueManager::EvqueueManager(int tenantId)
{
  std::cout << "EvqueueManager CTOR" << std::endl;
  mIposer = new Interposer();
  //Sending CUPTI profiling info
  /*std::string url("ipc:///tmp/pipeline.ipc");
  mComm = new Communicator(url, SENDER);
  mComm->connect();*/
  //assert((mSock = nn_socket(AF_SP, NN_PUSH)) >= 0);
  //assert(nn_connect (mSock, url.c_str()) >= 0);

  std::string req_url, resp_url;
  if (tenantId == 1)
  {  
     req_url = "ipc:///tmp/req1.ipc";
     resp_url = "ipc:///tmp/resp1.ipc";
  }
  else
  {
     req_url = "ipc:///tmp/req2.ipc";
     resp_url = "ipc:///tmp/resp2.ipc";
  }
  std::cout << "Creating sender at " << req_url  << " for tenant " << tenantId << std::endl; 
  std::cout << "Creating receiver at " << resp_url  << " for tenant " << tenantId << std::endl; 
  mReq = new Communicator(req_url, SENDER);
  mReq->connect();
  mResp = new Communicator(resp_url, RECEIVER);
  mResp->bind();

  std::string url("ipc:///tmp/pinfo.ipc");
  mComm = new Communicator(url, SENDER);
  mComm->connect();

  size_t attrValue = 0, attrValueSize = sizeof(size_t);
  attrValue = 32 * 1024 * 1024;
  //CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
  attrValue = 1;
  //CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));

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
  struct timeval now;
  gettimeofday(&now, NULL);
  mStartSysClock = now.tv_sec * 1000000 + now.tv_usec;
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

int EvqueueManager::launch(KernelIdentifier kid, unsigned long have_run_for, long service_id)
{
  return mIposer->launch(kid, have_run_for, service_id);
}

extern "C"
void EvqueueCreate(int tenantId)
{
    gEvqm = new EvqueueManager(tenantId);
}

extern "C"
void EvqueueDestroy()
{
    delete gEvqm;
}

extern "C"
void EvqueueLaunch(KernelIdentifier kid, unsigned long have_run_for, long service_id)
{
    gEvqm->launch(kid, have_run_for, service_id);
}

extern "C"
void EvqueueSynch()
{
    gEvqm->synch();
}
