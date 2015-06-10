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
       //mPerTenantLock.push_back(std::unique_ptr<std::mutex>(new std::mutex));
       //mPerTenantNotify.push_back(std::unique_ptr<std::condition_variable>(new std::condition_variable));
    }
};

void RequestWindow::addRequest(int tenantId, RequestDescriptor *reqDesc)
{
  mPerTenantRequestQueue[tenantId].push(reqDesc);
  //RequestDescriptor *temp = mPerTenantRequestQueue[tenantId].back();
  //std::cout << "Peeking at request (addRequest) ";
  //printReqDescriptor(temp);
}

bool RequestWindow::hasRequest(int tenantId)
{
    //std::cout << "Is queue empty for tenantId " << tenantId << "returned " << mPerTenantRequestQueue[tenantId].empty() << std::endl;
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
    lk.unlock();
}

void RequestWindow::sendResponse(int tenantId)
{
    std::lock_guard<std::mutex> lk(mPerTenantLock[tenantId]);
    mPerTenantReqReady[tenantId] = true;
    mPerTenantNotify[tenantId].notify_one();
}
