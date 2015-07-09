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

#include "RequestWindow.h"

RequestWindow::RequestWindow(int numTenants)
 : mTenants(numTenants), mPerTenantRequestQueue(numTenants), mPerTenantReqReady(numTenants, false)
{
    for(int i=0; i<numTenants; i++)
    {
       mPerTenantReqReady.push_back(new bool());
       mPerTenantServiceId.push_back(-1);
    }
};

void RequestWindow::lock()
{
   mMutex.lock();
}

void RequestWindow::unlock()
{
   mMutex.unlock();
}

void RequestWindow::addRequest(int tenantId, RequestDescriptor *reqDesc)
{
  mPerTenantRequestQueue[tenantId].push(reqDesc);
}

bool RequestWindow::hasRequest(int tenantId)
{
    if(mPerTenantRequestQueue[tenantId].empty())
    {
       return false;
    }
    return true;
}

RequestDescriptor* RequestWindow::peekRequest(int tenantId)
{
   return mPerTenantRequestQueue[tenantId].back();
}

RequestDescriptor* RequestWindow::consumeRequest(int tenantId)
{
    if(mPerTenantRequestQueue[tenantId].empty())
    {
       return NULL;
    }
    RequestDescriptor* reqDesc = mPerTenantRequestQueue[tenantId].front();
    mPerTenantRequestQueue[tenantId].pop();
    return reqDesc;
}

void RequestWindow::addRequestor(int tenantId)
{
    mWaitingTenantId.insert(tenantId);
}

void RequestWindow::removeRequestor(int tenantId)
{
   mWaitingTenantId.erase(tenantId);
}

void RequestWindow::waitForResponse(int tenantId)
{
    std::unique_lock<std::mutex> lk(mPerTenantLock[tenantId]);
    mPerTenantNotify[tenantId].wait(lk, [this, tenantId]{return mPerTenantReqReady[tenantId];});
    mPerTenantReqReady[tenantId] = false;
    lk.unlock();
}

void RequestWindow::sendResponse(int tenantId)
{
    std::lock_guard<std::mutex> lk(mPerTenantLock[tenantId]);
    mPerTenantReqReady[tenantId] = true;
    mPerTenantNotify[tenantId].notify_one();
}

int RequestWindow::getServiceId(int tenantId)
{
    long to_be_returned = mPerTenantServiceId[tenantId];
    mPerTenantServiceId[tenantId] = -1;
    return to_be_returned;
}

void RequestWindow::setServiceId(int tenantId, long serviceId)
{
    assert(mPerTenantServiceId[tenantId] == -1);
    mPerTenantServiceId[tenantId] = serviceId;
}
