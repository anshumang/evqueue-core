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
 : mNumTenants(numTenants), mSchedulingEpoch(schedulingEpoch), mPinfos(pinfos), mTenantBeingServiced(-1), mNumEpochsSilent(0)
{
   mReqWindow = new RequestWindow(numTenants);
   for (int count=0; count<numTenants; count++)
   {
      mPendingRequestorsAtleastOnce.push_back(false);
      m_request_ctr.push_back(0);
   }
   mTieBreaker[0] = false;
   mTieBreaker[1] = false;
   mTieBreaker[2] = false;
   mTieBreaker[3] = false;
   mTieBreaker[4] = false;
   mTieBreaker[5] = false;
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
          //std::cout << "[ARBITER] ------tick------" << std::endl;
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
                m_request_ctr[tenantId]++;
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
                unsigned long min_duration=0, max_duration=0;
		//std::cout << "ks(hasPinfo) : " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " " << ks.mBlockX << " " << ks.mBlockY << " " << ks.mBlockZ << std::endl;
                mPinfos.lock();
                bool found = mPinfos.hasPinfo(ks, &min_duration, &max_duration);
                mPinfos.unlock();
                if(found) //if profile info present from previous launch
                {
                   //std::cout << ks.mGridX << " " << duration << " " << m_request_ctr[tenantId] << " " << m_request_ctr[tenantId]%23 << std::endl;
                   //std::cout << "P " << tenantId << " " << /*mNumTenants**/duration << std::endl;
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==4))
                   {
                      mTieBreaker[0] = true;
                      //std::cout << "4" << std::endl;
                   }
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==6))
                   {
                      mTieBreaker[1] = true;
                      //std::cout << "6" << std::endl;
                   }
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==13))
                   {
                      mTieBreaker[2] = true;
                      //std::cout << "13 " << duration << std::endl;
                   }
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==14))
                   {
                      mTieBreaker[3] = true;
                      //std::cout << "14" << std::endl;
                   }
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==16))
                   {
                      mTieBreaker[4] = true;
                      //std::cout << "16" << std::endl;
                   }
                   if((tenantId == 0)&&(m_request_ctr[tenantId]%23==18))
                   {
                      mTieBreaker[5] = true;
                      //std::cout << "18" << std::endl;
                   }
                   if(mTieBreaker[0]||mTieBreaker[1]||mTieBreaker[2]||mTieBreaker[3]||mTieBreaker[4]||mTieBreaker[5])
                   {
                       //std::cout << "------" << std::endl;
                   }
                   if(tenantId == 0)
                   {
                     if(mTieBreaker[0]||mTieBreaker[1]||mTieBreaker[2]||mTieBreaker[3]||mTieBreaker[4]||mTieBreaker[5])
                     {
                        if(mTieBreaker[2])
                        {
                          min_duration = 2*min_duration;
                        }
                        min_duration = 2*min_duration;
                        //std::cout << "doubled" << std::endl;
                        mTieBreaker[0] = false;
                        mTieBreaker[1] = false;
                        mTieBreaker[2] = false;
                        mTieBreaker[3] = false;
                        mTieBreaker[4] = false;
                        mTieBreaker[5] = false;
                        q = std::make_pair(/*mNumTenants**/min_duration, tenantId);
                        std::cerr << ks.mGridX << " " << min_duration << std::endl;
                      }
                      else
                      {
                        q = std::make_pair(/*mNumTenants**/min_duration, tenantId);
                        std::cerr << ks.mGridX << " " << min_duration << std::endl;
                      }
                   }
                   if(tenantId != 0)
                   {
                      if(m_request_ctr[tenantId]%52>2)
                      {
                      q = std::make_pair(max_duration, tenantId);
                        std::cerr << ks.mGridX << " " << max_duration << std::endl;
                      }
                      else
                      {
                      q = std::make_pair(min_duration, tenantId);
                        std::cerr << ks.mGridX << " " << min_duration << std::endl;
                      }
                   }
                   //q = std::make_pair(/*mNumTenants**/duration, tenantId);
                   //std::cout << "[ARBITER] Pinfo deadline for " << tenantId << " with signature " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " at " << /*mNumTenants**/duration << std::endl;
                   //std::cout << ks.mGridX << " " << duration << std::endl;
                }
                else //else num of tenents x scheduling epoch
                {
                   //std::cout << "P " << tenantId << " " << /*mNumTenants**/mSchedulingEpoch << std::endl;
                   q = std::make_pair(/*mNumTenants**/mSchedulingEpoch, tenantId);
                   //std::cout << "[ARBITER] Naive deadline for " << tenantId << " with signature " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " at " << mNumTenants*mSchedulingEpoch << std::endl;
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
	  if(deadlinesPerTenant.size()>1 || !mPendingRequestorsSet.empty() || !mServicedRequestorsSet.empty())
	  {
		  if(!deadlinesPerTenant.empty())
		  {
			  //std::cout << "Deadlines " << deadlinesPerTenant.size() << " " << mPendingRequestorsSet.size() << " " << mServicedRequestorsSet.size() << " : ";
		  }
		  for (auto const& p : deadlinesPerTenant)
		  {
			  //std::cout << p.first << " ";
		  }
		  if(!deadlinesPerTenant.empty())
		  {
			  //std::cout << endl;
		  }
	  }

          //send responses
      #if 0
          for (auto const& p : deadlinesPerTenant)
          {
             //std::cout << "[ARBITER] Sending response to " << p.second << " with deadline " << p.first << std::endl;
             mReqWindow->sendResponse(p.second);
	     boost::posix_time::milliseconds subepoch(1);
             boost::this_thread::sleep(subepoch); //Space out the responses to avoid packet reordering
          }
      #endif


	  if (mPendingRequestorsSet.empty())
	  {
		  assert(mRequestorCumulatedServiceSliceMap.size() == 0);
		  /*pick a new tenant if there are new requests and nothing is pending*/
		  if(!deadlinesPerTenant.empty())
		  {
			  assert(mTenantBeingServiced == -1);//if picking one, this should have been invalidated in the last servicing
			  int minTenantId = mNumTenants + 1; //mNumTenants should be enough as tenants numbered starting 0
			  for (auto const& p : deadlinesPerTenant)
			  {
				  if(p.second < minTenantId)
				  {
					  minTenantId = p.second;
				  }
			  }
			  assert(minTenantId < mNumTenants); //if minTenantId still not a legal tenantId, deadlinesPerTenant is probably corrupted
			  mTenantBeingServiced = minTenantId;
			  //std::cout  << "Picking a tenant to be serviced " << mTenantBeingServiced << std::endl;
		  }
#if 0
		  if(!deadlinesPerTenant.empty() && mServicedRequestorsSet.empty()) /*only modify the slice until no tenant has been serviced in the current round*/
              {
                 auto const &l = deadlinesPerTenant.back();
                 mCurrServiceSlice = l.first;
                 if(deadlinesPerTenant.size()>1)
                 {
                    std::cout << "Slice " << mCurrServiceSlice << std::endl;
                 }
              }
              else
              {
                 mCurrServiceSlice = 0;
                 std::cout << "No pending requestor yet no new requestor" << std::endl;
              }
#endif
              //if(deadlinesPerTenant.empty())
              //{
                 mCurrServiceSlice = 0;
                 //std::cout << "No pending requestor yet no new requestor" << std::endl;
              //}
              mServicedRequestorsSet.clear();
              if(!mBlockedRequestorCumulatedServiceSliceMap.empty())
              {
                   unsigned long other_max_slice = 0;
                   int minTenantId = mNumTenants + 1;
                   //std::cout << "Blocked requestors ";
                   for (auto const& p: mBlockedRequestorCumulatedServiceSliceMap)
                   {
                       std::cout << p.first << " " << p.second;
                       if(p.second > other_max_slice)
                       {
                            other_max_slice = p.second;
                       }
                       if(p.first < minTenantId)
                       {
                            minTenantId = p.first;
                       }
                       mPendingRequestorsSet.insert(p.first);
                   }
                   if((mTenantBeingServiced >= 0)&&(minTenantId < mNumTenants)) /*some legal tenantId and we have a legal new minima among the blocked requests*/
                   {
                         if(minTenantId < mTenantBeingServiced)
                         {
                            mTenantBeingServiced = minTenantId;
                            mPendingRequestorsAtleastOnce[minTenantId] = true;
                         }
                   }
                   if((mTenantBeingServiced == -1)&&(minTenantId < mNumTenants))
                   {
                        mTenantBeingServiced = minTenantId;
                        mPendingRequestorsAtleastOnce[minTenantId] = true;
                   }
                   //if(other_max_slice > mCurrServiceSlice)
                   //{
                      mCurrServiceSlice = other_max_slice;
                   //}
                   //std::cout << " Slice(from blocked requests) " << other_max_slice << std::endl;
                   mRequestorCumulatedServiceSliceMap.insert(mBlockedRequestorCumulatedServiceSliceMap.begin(), mBlockedRequestorCumulatedServiceSliceMap.end());
                   mBlockedRequestorCumulatedServiceSliceMap.clear();
              }
              else
              {
                  //std::cout << "No blocked requestors" << std::endl;
              }
          }
	  if(!deadlinesPerTenant.empty() && mServicedRequestorsSet.empty()) /*can modify the slice until no tenant has been serviced in the current round*/
	  {
		  auto const &l = deadlinesPerTenant.back();
		  if( mCurrServiceSlice < l.first)
                  {
                       mCurrServiceSlice = l.first;
                  }
		  //if(deadlinesPerTenant.size()>1)
		  //{
			  //std::cout << "Slice (from new requests) " << mCurrServiceSlice << std::endl;
		  //}
	  }
	  //std::cout << " Slice(final) " << mCurrServiceSlice << std::endl;
          for (auto const& p : deadlinesPerTenant)
          {
                 /*only service this tenant, if not in servicedRequestorsSet*/
		  if (mServicedRequestorsSet.find(p.second) == mServicedRequestorsSet.end())
		  {
                          //std::cout << "Tenant not serviced in this round ";
			  auto did_insert = mPendingRequestorsSet.insert(p.second);
			  if (did_insert.second) /*new requestor*/
			  {
                                  assert(mPendingRequestorsAtleastOnce[p.second] == false);
                                  mPendingRequestorsAtleastOnce[p.second] = true;
                                  //std::cout << "New Requestor " << p.second << " " << p.first << " --- ";
				  auto r = std::make_pair(p.second, p.first);
				  mRequestorCumulatedServiceSliceMap.insert(r); 
			  }
			  else /*pending requestor*/
			  {
                                  //std::cout << "In service Requestor " << p.second << " " << p.first << " --- ";
				  auto r = std::make_pair(p.second, p.first);
                                  auto const& q = mRequestorCumulatedServiceSliceMap.find(p.second);
				  if(q != mRequestorCumulatedServiceSliceMap.end())
				  {
					  unsigned long updated_cumul_service_slice = (*q).second + p.first;
					  //std::cout << "In service Requestor (update) " << p.second << " " << (*q).second << " " << updated_cumul_service_slice << " --- ";
                                          auto r = std::make_pair(p.second, updated_cumul_service_slice);
                                          mRequestorCumulatedServiceSliceMap.erase(p.second);
					  mRequestorCumulatedServiceSliceMap.insert(r); 
				  }
                                  else
                                  {
                                    //this should not happen, any requestor present in pendingRequestorsSet should also be in the requestorCumulatedServiceSliceMap
                                    std::cout << "requestor present in pendingRequestorsSet but not in the requestorCumulatedServiceSliceMap" << std::endl;
                                    assert(0);
                                  }
			  }
		  }
                  else
                  {
                          //std::cout << "Tenant already serviced in this round " << p.second << " "  << p.first;
			  if(mBlockedRequestorCumulatedServiceSliceMap.find(p.second) == mBlockedRequestorCumulatedServiceSliceMap.end())
                          {
                                //std::cout << " added to blocked";
				auto r = std::make_pair(p.second, p.first);
                                mBlockedRequestorCumulatedServiceSliceMap.insert(r);
                          }
                          else
                          {
                                //this should not happen, the request from serviced tenant is blocked which means a second request from a serviced tenant can't arrive
                                std::cout << "second request from a serviced tenant" << std::endl;
                                assert(0);
                          }
                  }
	          //std::cout << std::endl;
          }
	  //auto const& r = mRequestorCumulatedServiceSliceMap.begin();

	  if(!mPendingRequestorsSet.empty())
	  {
		  assert(mTenantBeingServiced != -1);
		  auto const& r = mRequestorCumulatedServiceSliceMap.find(mTenantBeingServiced);
		  if (r != mRequestorCumulatedServiceSliceMap.end())
		  {
			  auto const& key = (*r).first;
			  auto const& value = (*r).second; 
			  //if mTenantBeingServiced not in deadlinesPerTenant and mPendingRequestorsSet.size() > 1, missed scheduling opportunity
			  bool tenantBeingServicedHasRequest = false;
			  for(auto const& p : deadlinesPerTenant)
			  {
                                  //std::cout << p.second << " " << mTenantBeingServiced << std::endl;
				  if(p.second == mTenantBeingServiced)
                                  {
                                          tenantBeingServicedHasRequest = true;
					  break;
                                  }
			  }
			  if(tenantBeingServicedHasRequest || mPendingRequestorsAtleastOnce[key])
			  {
                                  if(mPendingRequestorsAtleastOnce[key])
                                  {
                                      mPendingRequestorsAtleastOnce[key] = false;
                                  }
				  //std::cout << "Now servicing " << key << " " << value << std::endl;
				  mReqWindow->sendResponse(key);
                                  if((key>0)&&(value > mCurrServiceSlice))
                                  {
                                      std::cout << "nnf losing gpu by " << value-mCurrServiceSlice << " " << value << " " << mCurrServiceSlice << std::endl; 
                                  }
				  if(value >= mCurrServiceSlice) /*cumulServiceSlice higher than any pending requestor*/
				  {
					  //std::cout << "Done " << key << " with slice of " << mCurrServiceSlice << std::endl;
					  mRequestorCumulatedServiceSliceMap.erase(key);
					  /*Find the next tenant to be serviced*/
					  auto const& s = mPendingRequestorsSet.upper_bound(mTenantBeingServiced);
					  if(s == mPendingRequestorsSet.end())
					  {
						  auto const& t = mPendingRequestorsSet.lower_bound(mTenantBeingServiced);
						  if(t == mPendingRequestorsSet.begin())
						  {
							  //std::cout << "Was the only tenant active" << std::endl;
							  mTenantBeingServiced = -1; /*set to an illegal value for error checking*/
                                                          //std::cout << "No next tenant" << std::endl;
						  }
						  else
						  {
							  std::set<int>::iterator tprime = t;
							  unsigned int nextTenant = *(tprime++);
							  //std::cout << "Was active " << mTenantBeingServiced << " now active " << nextTenant << std::endl;
							  mTenantBeingServiced = nextTenant;
						  }
					  }
					  else
					  {
						  unsigned int nextTenant = *s;
						  //std::cout << "Was active " << mTenantBeingServiced << " now active " << nextTenant << std::endl;
						  mTenantBeingServiced = nextTenant;
					  }
					  mPendingRequestorsSet.erase(key);
					  mServicedRequestorsSet.insert(key);

				  }
				  else
				  {
					  //std::cout << "Continued " << key << " with slice of " << mCurrServiceSlice << " still remaining " <<  mCurrServiceSlice - value << std::endl;
				  }
				  mNumEpochsSilent = 0;
			  }
			  else
			  {
				  //can't wait forever to ensure fairness, not a lot more than the last requested duration accumulated (we don't store this separately, only the cumulative sum in mRequestorCumulatedServiceSliceMap), probably the upper bound is the cumulative sum - epoch if all requests came in a burst (-epoch is atleast the time it got since it was scheduled)
				  //let's count how many epochs we wait for the next request from this tenant
				  mNumEpochsSilent++;
				  std::cout << "Silent for " << key << " with accumulated " << value << " and this is the " << mNumEpochsSilent << "th allowance when others waiting are ";
				  for(auto const& u : mRequestorCumulatedServiceSliceMap)
				  {
					  if(u.first == key)
						  continue;
					  std::cout << u.first << " " << u.second;
				  }
				  std::cout << std::endl;
			  }
		  }
		  else
		  {
			  //tenantBeingServiced does a handoff to a next valid tenant unless nothing pending
			  assert(0); 
			  //std::cout << "tenantBeingServiced does a handoff to a next valid tenant unless nothing pending "  << std::endl;
		  }
	  }
              
	  boost::this_thread::interruption_point(); //otherwise this thread can't be terminated
  } 
}

