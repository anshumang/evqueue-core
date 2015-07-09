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

#include <map>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <set>
#include <mutex>
#include <condition_variable>
#include <utility>
//#include <memory> //For unique_ptr and shared_ptr
#include <array>
#include <algorithm>
#include <cassert>
#include "ReqRespDescriptor.h"

struct RequestWindow
{
  int mTenants;
  std::vector <std::queue <RequestDescriptor *> > mPerTenantRequestQueue;
  std::set <int> mWaitingTenantId;
  std::vector<bool> mPerTenantReqReady;
  std::vector<long> mPerTenantServiceId;
  std::array<std::mutex, 2> mPerTenantLock;
  std::array<std::condition_variable, 2> mPerTenantNotify;
  std::mutex mMutex;
  RequestWindow(int numTenants);
  void lock();
  void unlock();
  //mPerTenantRequestQueue helpers
  void addRequest(int tenantId, RequestDescriptor *reqDesc);
  bool hasRequest(int tenantId);
  RequestDescriptor* peekRequest(int tenantId);
  RequestDescriptor* consumeRequest(int tenantId);
  //mWaitingTenantId helpers
  void addRequestor(int tenantId);
  void removeRequestor(int tenantId);
  void waitForResponse(int tenantId);
  void sendResponse(int tenantId);
  int getServiceId(int tenantId);
  void setServiceId(int tenantId, long serviceId);
};
