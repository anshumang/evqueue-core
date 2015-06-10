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

Reqresp::Reqresp(std::string req_url, std::string resp_url, int tenantId, Arbiter *arb)
  : mTenantId(tenantId), mArb(arb)
{
     std::cout << "Creating listener at " << req_url << std::endl;
     mReqComm = new Communicator(req_url, RECEIVER);
     mReqComm->bind();

     std::cout << "Creating responder at " << resp_url << std::endl;
     mRespComm = new Communicator(resp_url, SENDER);
     mRespComm->connect();
}

Reqresp::~Reqresp()
{
   delete mReqComm;
   delete mRespComm;
}

void Reqresp::start()
{
   mThread = boost::thread(&Reqresp::ProcessReq, this);
}

void Reqresp::join()
{
   mThread.join();
}

void Reqresp::SendResponse()
{
   ResponseDescriptor respDesc;
   respDesc.mNeedYield = false;
   respDesc.mRunSlice = 10000000; //10 ms
   mRespComm->send(&respDesc, sizeof(ResponseDescriptor)); 
}

void Reqresp::ProcessReq()
{
   std::cout << "Daemon starts listening at " << mReqComm->mSock  << " with URL of " << mReqComm->mURL << std::endl;
   while(true)
   {
      void *buf = NULL;
      int bytes = mReqComm->receive(&buf);
      assert(bytes >= 0);

      /*
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
      */

      RequestDescriptor *reqDesc = (RequestDescriptor*)buf;
      //std::cout << "Adding request ";
      //printReqDescriptor(reqDesc);
      //std::cout << " from " << mTenantId << std::endl;
      mArb->mReqWindow->addRequest(mTenantId, reqDesc);  
      /*auto tenantId = 0;
      for (auto const& p : mArb->mReqWindow->mPerTenantRequestQueue)
      {
	  if(mArb->mReqWindow->hasRequest(tenantId))
	  {
	      RequestDescriptor *reqDesc = mArb->mReqWindow->peekRequest(tenantId);
	      std::cout << "Peeking at request(Reqresp thread) ";
              printReqDescriptor(reqDesc);
	      std::cout << " from " << tenantId << std::endl;
          }
	  tenantId++;
      }*/
      //mArb->mReqWindow->addRequestor(mTenantId);
      //the blocking in tenant is a result of delaying the receive()
      mArb->mReqWindow->waitForResponse(mTenantId);
      std::cout << "[REQRESP] Tenant " << mTenantId << " received response" << std::endl; 
      SendResponse();
      mReqComm->freemsg(buf);
   }
}
