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

#ifndef _ARBITER_H
#define _ARBITER_H

#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include "RequestWindow.h"
#include "PinfoStore.h"

struct Arbiter
{
   Arbiter(int numTenants, unsigned long schedulingEpoch, PinfoStore& pinfos);
   ~Arbiter();
   void start();
   void join();
   void ProcessQueue();
   boost::thread mThread;
   RequestWindow *mReqWindow;   
   friend struct Reqresp;
   PinfoStore& mPinfos;
   int mNumTenants;
   unsigned long mSchedulingEpoch;
   unsigned long mCurrServiceSlice;
   int mTenantBeingServiced, mNumEpochsSilent;
   std::set<int> mPendingRequestorsSet, mServicedRequestorsSet;
   std::map<int, unsigned long> mRequestorCumulatedServiceSliceMap, mBlockedRequestorCumulatedServiceSliceMap, mRequestorCurrentAllottedServiceSliceMap;
   std::vector<bool> mPendingRequestorsAtleastOnce;
   std::vector<unsigned int>m_request_ctr;
   std::array<bool, 6> mTieBreaker;
};

#endif
