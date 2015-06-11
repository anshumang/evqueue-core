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

#include "Arbiter.h"

Arbiter::Arbiter(int numTenants, unsigned long schedulingEpoch, PinfoStore& pinfos)
 : mNumTenants(numTenants), mSchedulingEpoch(schedulingEpoch), mPinfos(pinfos)
{
   mReqWindow = new RequestWindow(numTenants);
}

Arbiter::~Arbiter()
{
   delete mReqWindow;
}

void Arbiter::start()
{
   std::cout << "In ProcessQueue thread" << std::endl;
   mThread = boost::thread(&Arbiter::ProcessQueue, this);
}

void Arbiter::join()
{
   mThread.join();
}

void Arbiter::ProcessQueue()
{
  boost::posix_time::milliseconds epoch(10);
  while(true) //runs forever
  {
	  boost::this_thread::sleep(epoch);
          std::cout << "[ARBITER] ------tick------" << std::endl;
	  //queue gets processed here
          auto tenantId = 0;
          std::vector<std::pair<unsigned long, int>> deadlinesPerTenant;
          
          for (auto const& p : mReqWindow->mPerTenantRequestQueue)
          {
            mReqWindow->lock();
            RequestDescriptor *reqDesc = mReqWindow->consumeRequest(tenantId);
            mReqWindow->unlock();
            if(reqDesc)
            //if(mReqWindow->hasRequest(tenantId))
            {
                //RequestDescriptor *reqDesc = mReqWindow->peekRequest(tenantId);
                //std::cout << "Peeking at request(Arbiter thread) ";
                //printReqDescriptor(reqDesc);
                //std::cout << " from " << tenantId << std::endl;
                std::pair<unsigned long, int> q;
                struct KernelSignature ks;
                ks.mGridX = reqDesc->grid[0];
                ks.mGridY = reqDesc->grid[1];
                ks.mGridZ = reqDesc->grid[2];
                ks.mBlockX = reqDesc->block[0];
                ks.mBlockY = reqDesc->block[1];
                ks.mBlockZ = reqDesc->block[2];
                unsigned long duration=0;
		//std::cout << "ks(hasPinfo) : " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " " << ks.mBlockX << " " << ks.mBlockY << " " << ks.mBlockZ << std::endl;
                mPinfos.lock();
                bool found = mPinfos.hasPinfo(ks, &duration);
                mPinfos.unlock();
                if(found) //if profile info present from previous launch
                {
                   q = std::make_pair(mNumTenants*duration, tenantId);
                   std::cout << "[ARBITER] Pinfo deadline for " << tenantId << " with signature " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " at " << /*mNumTenants**/duration << std::endl;
                }
                else //else num of tenents x scheduling epoch
                {
                   q = std::make_pair(mNumTenants*mSchedulingEpoch, tenantId);
                   std::cout << "[ARBITER] Naive deadline for " << tenantId << " with signature " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " at " << mNumTenants*mSchedulingEpoch << std::endl;
                }
                deadlinesPerTenant.push_back(q); //for now, based on order of arrival
            }
            else
            {
                 //std::cout << "[ARBITER] No request found from " << tenantId << std::endl;
            }
            tenantId++;
          }

          //scheduling policy
          std::sort(deadlinesPerTenant.begin(), deadlinesPerTenant.end());

          //send responses
          for (auto const& p : deadlinesPerTenant)
          {
             std::cout << "[ARBITER] Sending response to " << p.second << " with deadline " << p.first << std::endl;
             mReqWindow->sendResponse(p.second);
	     boost::posix_time::milliseconds subepoch(1);
             boost::this_thread::sleep(subepoch); //Space out the responses to avoid packet reordering
          }
	  boost::this_thread::interruption_point(); //otherwise this thread can't be terminated
  } 
}

