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

Arbiter::Arbiter(int numTenants, PinfoStore& pinfos)
 : mPinfos(pinfos)
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
          std::cout << "Epoch elapsed" << std::endl;
	  //queue gets processed here
          auto tenantId = 0;
          std::vector<std::pair<unsigned long, int>> deadlinesPerTenant;
          
          for (auto const& p : mReqWindow->mPerTenantRequestQueue)
          {
            if(mReqWindow->hasRequest(tenantId))
            {
                std::cout << "Peeking at request ";
                printReqDescriptor(mReqWindow->peekRequest(tenantId));
                std::cout << " from " << tenantId << std::endl;
                auto q = std::make_pair(mReqWindow->peekRequest(tenantId)->timestamp, tenantId);
                deadlinesPerTenant.push_back(q); //for now, arrivalsPerTenant
            }
            else
            {
                 std::cout << "No request found from " << tenantId << std::endl;
            }
            tenantId++;
          }

          //scheduling policy
          std::sort(deadlinesPerTenant.begin(), deadlinesPerTenant.end());

          //send responses
          for (auto const& p : deadlinesPerTenant)
          {
             std::cout << "Sending response to " << p.second << std::endl;
             mReqWindow->sendResponse(p.second);
	     boost::posix_time::milliseconds subepoch(1);
             boost::this_thread::sleep(subepoch); //Space out the responses to avoid packet reordering
          }
	  boost::this_thread::interruption_point(); //otherwise this thread can't be terminated
  } 
}

