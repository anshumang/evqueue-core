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

RequestWindow::RequestWindow()
 : mPerTenantRequestQueue(2), mPerTenantReqReady(2, false)
{

};

void RequestWindow::addRequest(int tenantId, RequestDescriptor *reqDesc)
{
  std::cout << "Adding a request from tenant " << tenantId << std::endl;
  mPerTenantRequestQueue[tenantId].push(reqDesc);
}

RequestDescriptor* RequestWindow::peekRequest(int tenantId)
{
   return mPerTenantRequestQueue[tenantId].front();
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

void RequestWindow::releaseRequestor(int tenantId)
{
    std::lock_guard<std::mutex> lk(mPerTenantLock[tenantId]);
    mPerTenantReqReady[tenantId] = true;
    mPerTenantNotify[tenantId].notify_one();
}
