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

#include "Reqresp.h"

Reqresp::Reqresp(std::string url, int tenantId, Arbiter *arb)
  : mTenantId(tenantId), mArb(arb)
{
   std::cout << "Creating listener at " << url << std::endl;
   mComm = new Communicator(url, RECEIVER);
   mComm->bind();
}

Reqresp::~Reqresp()
{
   delete mComm;
}

void Reqresp::start()
{
   mThread = boost::thread(&Reqresp::ProcessReq, this);
}

void Reqresp::join()
{
   mThread.join();
}

void Reqresp::ProcessReq()
{
   std::cout << "Daemon starts listening at " << mComm->mSock  << " with URL of " << mComm->mURL << std::endl;
   while(true)
   {
      void *buf = NULL;
      int bytes = mComm->receive(&buf);
      assert(bytes >= 0);
      RequestDescriptor *reqDesc = (RequestDescriptor*)buf;
      mArb->mReqWindow->addRequest(mTenantId, reqDesc);  
      mArb->mReqWindow->addToWaiters(mTenantId);
      struct timeval now;
      gettimeofday(&now, NULL);
      std::cout
	      << "Tenant  : " << mTenantId << " "
	      << now.tv_sec * 1000000 + now.tv_usec << " "
	      << now.tv_sec * 1000000 + now.tv_usec - reqDesc->timestamp << " "
	      << reqDesc->grid[0] << " "
	      << reqDesc->grid[1] << " "
	      << reqDesc->grid[2] << " "
	      << reqDesc->block[0] << " "
	      << reqDesc->block[1] << " "
	      << reqDesc->block[2] << " "
	      << bytes
	      << std::endl;

      mComm->freemsg(buf);
   }
}
