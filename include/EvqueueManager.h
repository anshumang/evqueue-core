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

#ifndef _EVQUEUE_MANAGER_H_
#define _EVQUEUE_MANAGER_H_

#include <cassert>
#include "CuptiCallbacks.h"
//#include "EvqueueClient.h"
#include "Messages.h"
#include "Interposer.h"
#include "Communicator.h"

struct EvqueueManager
{
  //EvqueueClient *m_evqueue_client;
  Interposer *mIposer;
  Communicator *mComm, *mReq, *mResp;
  unsigned long mStartSysClock;
  EvqueueManager(int tenantId);
  ~EvqueueManager(void);
  void synch(void);
  int launch(KernelIdentifier kid);
  friend void CUPTIAPI bufferCompleted(CUcontext, uint32_t, uint8_t *, size_t, size_t);
};

extern EvqueueManager *gEvqm;

extern "C" void EvqueueCreate(int tenantId);
extern "C" void EvqueueDestroy();
extern "C" void EvqueueLaunch(KernelIdentifier k);
extern "C" void EvqueueSynch();

#endif
