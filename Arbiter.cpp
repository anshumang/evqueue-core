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
  boost::posix_time::milliseconds epoch(1);
  while(true) //runs forever
  {
	  boost::this_thread::sleep(epoch);
	  //queue gets processed here
          auto tenantId = 0;
          /*pair.first can't be unsigned to handle -ve for inservice requests*/
          std::vector<std::pair<long, int>> deadlinesPerTenant;
          
          for (auto const& p : mReqWindow->mPerTenantRequestQueue)
          {
            mReqWindow->lock();
            RequestDescriptor *reqDesc = mReqWindow->consumeRequest(tenantId);
            mReqWindow->unlock();
            if(reqDesc)
            {
                std::pair<long, int> q;
                struct KernelSignature ks;
                ks.mGridX = reqDesc->grid[0];
                ks.mGridY = reqDesc->grid[1];
                ks.mGridZ = reqDesc->grid[2];
                ks.mBlockX = reqDesc->block[0];
                ks.mBlockY = reqDesc->block[1];
                ks.mBlockZ = reqDesc->block[2];
                unsigned long min_duration=0, max_duration=0;
                if((reqDesc->service_id != -1)&&(reqDesc->have_run_for > 0)) /*In service request - was 'yield'-ed, shouldn't lookup predictor and compute remaining time, instead*/
                {
                    long tot_run_time = reqDesc->service_id; // Should ideally use the serviceId to lookup the stored prediction, but since the serviceId is otherwise unused on client, using it to store the same information in transit
                    long have_run_for = reqDesc->have_run_for;
                    q = std::make_pair(tot_run_time - have_run_for, tenantId);
                    std::cout << "In service request arb " << tenantId << " " << reqDesc->service_id << " " << reqDesc->have_run_for << std::endl;
                }
                else /*New request, look up predictor*/
		{
			mPinfos.lock();
			bool found = mPinfos.hasPinfo(ks, &min_duration, &max_duration);
			mPinfos.unlock();
			if(found) //if profile info present from previous launch
			{
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==3))
				{
					mTieBreaker[0] = true;
				}
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==5))
				{
					mTieBreaker[1] = true;
				}
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==12))
				{
					mTieBreaker[2] = true;
				}
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==13))
				{
					mTieBreaker[3] = true;
				}
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==15))
				{
					mTieBreaker[4] = true;
				}
				if((tenantId == 0)&&(m_request_ctr[tenantId]%23==17))
				{
					mTieBreaker[5] = true;
				}
                                #if 0 /*Only for NNF/IN*/
				if(tenantId == 0)
				{
                                        if((m_request_ctr[tenantId]%23==14)&&(ks.mGridX != 192))
                                        {
                                            std::cout << "This should be 192 " << min_duration << std::endl;
                                        }
					if(mTieBreaker[0]||mTieBreaker[1]||mTieBreaker[2]||mTieBreaker[3]||mTieBreaker[4]||mTieBreaker[5])
					{
						if(mTieBreaker[2])
						{
							min_duration = 2*min_duration;
						}
						min_duration = 2*min_duration;
						mTieBreaker[0] = false;
						mTieBreaker[1] = false;
						mTieBreaker[2] = false;
						mTieBreaker[3] = false;
						mTieBreaker[4] = false;
						mTieBreaker[5] = false;
						q = std::make_pair(/*mNumTenants**/min_duration, tenantId);
						std::cerr << tenantId << " " << m_request_ctr[tenantId] << " 2x " << ks.mGridX << " " << min_duration << std::endl;
						mReqWindow->setServiceId(min_duration, tenantId);
					}
					else
					{
						q = std::make_pair(/*mNumTenants**/min_duration, tenantId);
						std::cerr << tenantId << " " << m_request_ctr[tenantId] << " x " << ks.mGridX << " " << min_duration << std::endl;
						mReqWindow->setServiceId(min_duration, tenantId);
					}
				}
                                #endif
				if(true/*tenantId != 0*/) /*For all, if NNT/IN is not 0*/
				{
					if(m_request_ctr[tenantId]%52>1)
					{
						q = std::make_pair(max_duration, tenantId);
						//std::cerr << tenantId << " " << m_request_ctr[tenantId] << " max " << ks.mGridX << " " << max_duration << std::endl;
						mReqWindow->setServiceId(max_duration, tenantId);
					}
					else
					{
						q = std::make_pair(min_duration, tenantId);
						//std::cerr << tenantId << " " << m_request_ctr[tenantId] << " min " << ks.mGridX << " " << min_duration << std::endl;
						mReqWindow->setServiceId(min_duration, tenantId);
					}
				}
				m_request_ctr[tenantId]++;
			}
			else //else num of tenents x scheduling epoch
			{
				q = std::make_pair(/*mNumTenants**/mSchedulingEpoch, tenantId);
				//std::cerr << tenantId << " " << ks.mGridX << " " << mSchedulingEpoch << std::endl;
				mReqWindow->setServiceId(mSchedulingEpoch, tenantId);
			}
		}
                deadlinesPerTenant.push_back(q); //for now, based on order of arrival
                std::cout << "[REQUEST PRESENT] " << q.first << " " << q.second << std::endl; 
            }
            tenantId++;
          }

          //scheduling policy
          std::sort(deadlinesPerTenant.begin(), deadlinesPerTenant.end());

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
          if(deadlinesPerTenant.empty())
          {
             //std::cout << "[REQUEST NONE]" << std::endl;
          }

	  if (mPendingRequestorsSet.empty())
	  {
		  assert(mRequestorCumulatedServiceSliceMap.size() == 0);
		  assert(mRequestorCurrentAllottedServiceSliceMap.size() == 0);
		  /*pick a new tenant if there are new requests and nothing is pending*/
		  if(!deadlinesPerTenant.empty())
		  {
			  assert(mTenantBeingServiced == -1);//if picking one, this should have been invalidated in the last servicing
			  int minTenantId = mNumTenants + 1; //mNumTenants should be enough as tenants numbered starting 0
			  std::cout << "PENDING NONE->DEADLINES ";
			  for (auto const& p : deadlinesPerTenant)
			  {
				  std::cout << p.first << " " << p.second;
				  if(p.second < minTenantId)
				  {
					  minTenantId = p.second;
				  }
				  std::cout << std::endl;
			  }
			  assert(minTenantId < mNumTenants); //if minTenantId still not a legal tenantId, deadlinesPerTenant is probably corrupted
			  mTenantBeingServiced = minTenantId;
			  std::cout  << "PENDING NONE->DEADLINES->TENANT " << mTenantBeingServiced << std::endl;
		  }
                  else
                  {
                  //std::cout << "PENDING NONE->DEADLINES NONE" << std::endl;
                  }
                 mCurrServiceSlice = 0;
                
               if(mServicedRequestorsSet.size()>0)
               {
               std::cout << "PENDING NONE->SERVICED ";
               for (auto const& p : mServicedRequestorsSet)
               {
                    std::cout << p << " ";
               }
               std::cout << std::endl;
               }
               else
               {
               //std::cout << "PENDING NONE->SERVICED NONE" << std::endl;
               }
              mServicedRequestorsSet.clear();
              if(!mBlockedRequestorCumulatedServiceSliceMap.empty())
              {
                   unsigned long other_max_slice = 0;
                   int minTenantId = mNumTenants + 1;
                   std::cout << "PENDING NONE->BLOCKED ";
                   for (auto const& p: mBlockedRequestorCumulatedServiceSliceMap)
                   {
                       std::cout << p.first << " " << p.second << " ";
                       if(p.second > other_max_slice)
                       {
                            other_max_slice = p.second;
                       }
                       if(p.first < minTenantId)
                       {
                            minTenantId = p.first;
                       }
                       mPendingRequestorsSet.insert(p.first);
                       mPendingRequestorsAtleastOnce[p.first] = true;
                   }
                   std::cout << std::endl;
                   if((mTenantBeingServiced >= 0)&&(minTenantId < mNumTenants)) /*some legal tenantId and we have a legal new minima among the blocked requests*/
                   {
                         if(minTenantId < mTenantBeingServiced)
                         {
                            mTenantBeingServiced = minTenantId;
                            //mPendingRequestorsAtleastOnce[minTenantId] = true;
                         }
                   }
                   if((mTenantBeingServiced == -1)&&(minTenantId < mNumTenants))
                   {
                        mTenantBeingServiced = minTenantId;
                        //mPendingRequestorsAtleastOnce[minTenantId] = true;
                   }
                   if(other_max_slice > 0)
                   {
                       mCurrServiceSlice = other_max_slice;
                   }
                   std::cout << "PENDING NONE->BLOCKED->SLICE " << mCurrServiceSlice << " PENDING NONE->BLOCKED->TENANT " << mTenantBeingServiced << std::endl;
                   mRequestorCumulatedServiceSliceMap.insert(mBlockedRequestorCumulatedServiceSliceMap.begin(), mBlockedRequestorCumulatedServiceSliceMap.end());
                   mRequestorCurrentAllottedServiceSliceMap.insert(mBlockedRequestorCumulatedServiceSliceMap.begin(), mBlockedRequestorCumulatedServiceSliceMap.end());
                   mBlockedRequestorCumulatedServiceSliceMap.clear();
              }
              else
              {
                  //std::cout << "PENDING NONE->BLOCKED NONE" << std::endl;
              }
          }
          else
          {
               std::cout << "PENDING ";
               for (auto const& p : mPendingRequestorsSet)
               {
                    std::cout << p << " ";
               }
               std::cout << std::endl;
          }
	  if(!deadlinesPerTenant.empty() && mServicedRequestorsSet.empty()) /*can modify the slice until no tenant has been serviced in the current round*/
	  {
		  auto const &l = deadlinesPerTenant.back();
		  if( mCurrServiceSlice < l.first)
                  {
                       std::cout << "UPDATE SLICE " << mCurrServiceSlice << " " << l.first << std::endl;
                       mCurrServiceSlice = l.first;
                  }
	  }
          for (auto const& p : deadlinesPerTenant)
          {
                 /*only service this tenant, if not in servicedRequestorsSet*/
		  if (mServicedRequestorsSet.find(p.second) == mServicedRequestorsSet.end())
		  {
			  auto did_insert = mPendingRequestorsSet.insert(p.second);
			  if (did_insert.second) /*new requestor*/
			  {
                                  assert(mPendingRequestorsAtleastOnce[p.second] == false);
                                  mPendingRequestorsAtleastOnce[p.second] = true;
                                  std::cout << "DEADLINES->NOT ALREADY SERVICED->FIRST " << p.second << " " << p.first << std::endl;
				  auto r = std::make_pair(p.second, p.first);
				  mRequestorCumulatedServiceSliceMap.insert(r); 
                                  auto rr = std::make_pair(p.second, p.first);
                                  mRequestorCurrentAllottedServiceSliceMap.insert(rr);
			  }
			  else /*pending requestor*/
			  {
				  auto r = std::make_pair(p.second, p.first);
                                  auto const& q = mRequestorCumulatedServiceSliceMap.find(p.second);
                                  auto const& qq = mRequestorCurrentAllottedServiceSliceMap.find(p.second);
				  if(q != mRequestorCumulatedServiceSliceMap.end())
				  {
                                          assert(qq != mRequestorCurrentAllottedServiceSliceMap.end());
					  unsigned long updated_cumul_service_slice = (*q).second + p.first;
					  std::cout << "DEADLINES->NOT ALREADY SERVICED->SECOND... " << p.second << " " << p.first << " " << (*q).second << " " << updated_cumul_service_slice << std::endl;
                                          auto r = std::make_pair(p.second, updated_cumul_service_slice);
                                          mRequestorCumulatedServiceSliceMap.erase(p.second);
					  mRequestorCumulatedServiceSliceMap.insert(r); 
                                          auto rr = std::make_pair(p.second, p.first);
                                          mRequestorCurrentAllottedServiceSliceMap.erase(p.second);
                                          mRequestorCurrentAllottedServiceSliceMap.insert(rr);
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
			  if(mBlockedRequestorCumulatedServiceSliceMap.find(p.second) == mBlockedRequestorCumulatedServiceSliceMap.end())
                          {
				std::cout << "DEADLINES->SERVICED->BLOCKED " << p.second << " "  << p.first << std::endl;
				auto r = std::make_pair(p.second, p.first);
                                std::cout << "DEADLINES->SERVICED->BLOCKED size (before insert) " << mBlockedRequestorCumulatedServiceSliceMap.size() << std::endl;
                                //auto const &ret = mBlockedRequestorCumulatedServiceSliceMap.insert(r);
                                mBlockedRequestorCumulatedServiceSliceMap.insert(std::pair<int, long>(p.second, p.first));
                                std::cout << "DEADLINES->SERVICED->BLOCKED size (after insert) " << mBlockedRequestorCumulatedServiceSliceMap.size() << std::endl;
                                /*if(ret.second==false)
                                {
                                  std::cout << "Insert failed because key " << ret.first->first << " already existed with a value of " << ret.first->second << std::endl;
                                }*/
				std::cout << "DEADLINES->SERVICED->BLOCKED ";
				/*for (auto const& p: mBlockedRequestorCumulatedServiceSliceMap)
				{
					std::cout << p.first << " " << p.second;
				}*/
                                int count=0;
                                for(std::map<int, long>::iterator it=mBlockedRequestorCumulatedServiceSliceMap.begin(); it!=mBlockedRequestorCumulatedServiceSliceMap.end(); ++it)
                                {
                                    count++;
                                    std::cout << it->first << " " << it->second << std::endl;
                                }
				std::cout << "size " << count << std::endl;
                          }
                          else
                          {
                                //this should not happen, the request from serviced tenant is blocked which means a second request from a serviced tenant can't arrive
                                std::cout << "second request from a serviced tenant" << std::endl;
                                assert(0);
                          }
                  }
          }

	  if(!mPendingRequestorsSet.empty())
	  {
		  assert(mTenantBeingServiced != -1);
		  auto const& r = mRequestorCumulatedServiceSliceMap.find(mTenantBeingServiced);
		  auto const& rr = mRequestorCurrentAllottedServiceSliceMap.find(mTenantBeingServiced);
		  if (r != mRequestorCumulatedServiceSliceMap.end())
		  {
                          assert(rr != mRequestorCurrentAllottedServiceSliceMap.end());
			  auto const& key = (*r).first;
			  auto const& value = (*r).second; 
                          auto const& valuevalue = (*rr).second;
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
                                  std::cout << "RESPONSE OK-> " << key << " " << tenantBeingServicedHasRequest << " " << mPendingRequestorsAtleastOnce[key] << " " << value << " " << valuevalue << " " << mCurrServiceSlice << std::endl;
                                  if(mPendingRequestorsSet.size()>1)
                                  {
                                     std::cout << "RESPONSE OK->OTHERS ";
                                     for(auto const& p : mPendingRequestorsSet)
                                     {
                                        if(p != key)
                                        {
                                           std::cout << p << " " << mRequestorCumulatedServiceSliceMap[p] << " " << mRequestorCurrentAllottedServiceSliceMap[p] << " ";
                                        }
                                     }
                                     std::cout << std::endl;
                                  }
                                  if(mPendingRequestorsAtleastOnce[key])
                                  {
                                      mPendingRequestorsAtleastOnce[key] = false;
                                  }
				  mReqWindow->sendResponse(key);
				  if((value >= mCurrServiceSlice)||(value<0)) /*cumulServiceSlice higher than any pending requestor*/
				  {
					  mRequestorCumulatedServiceSliceMap.erase(key);
					  mRequestorCurrentAllottedServiceSliceMap.erase(key);
					  /*Find the next tenant to be serviced*/
					  auto const& s = mPendingRequestorsSet.upper_bound(mTenantBeingServiced);
					  if(s == mPendingRequestorsSet.end())
					  {
						  auto const& t = mPendingRequestorsSet.lower_bound(mTenantBeingServiced);
						  if(t == mPendingRequestorsSet.begin())
						  {
							  std::cout << "RESPONSE OK->WAS ONLY TENANT SO -1" << std::endl;
							  mTenantBeingServiced = -1; /*set to an illegal value for error checking*/
						  }
						  else
						  {
							  std::set<int>::iterator tprime = t;
							  std::set<int>::iterator tprimeminus1 = t;
							  --tprimeminus1;
							  std::set<int>::iterator tprimeplus1 = t;
							  ++tprimeplus1;
							  std::cout << "RESPONSE OK->NEXT TENANT WHICH IS NOT LESS THAN " << *tprime << " " << *tprimeminus1 << " " << *tprimeplus1 << std::endl;
							  //unsigned int nextTenant = *(tprime++);
							  unsigned int nextTenant = *(tprimeminus1);
							  mTenantBeingServiced = nextTenant;
						  }
					  }
					  else
					  {
						  std::cout << "RESPONSE OK->NEXT TENANT IS GREATER THAN" << std::endl;
						  unsigned int nextTenant = *s;
						  mTenantBeingServiced = nextTenant;
					  }
					  std::cout << "RESPONSE OK->CHANGE TENANT " << mTenantBeingServiced << std::endl;
					  mPendingRequestorsSet.erase(key);
					  mServicedRequestorsSet.insert(key);

				  }
				  mNumEpochsSilent = 0;
			  }
			  else
			  {
				  //can't wait forever to ensure fairness, not a lot more than the last requested duration accumulated (we don't store this separately, only the cumulative sum in mRequestorCumulatedServiceSliceMap), probably the upper bound is the cumulative sum - epoch if all requests came in a burst (-epoch is atleast the time it got since it was scheduled)
				  //let's count how many epochs we wait for the next request from this tenant
				  mNumEpochsSilent++;
                                  std::cout << "RESPONSE NONE-> " << key << " " << tenantBeingServicedHasRequest << " " << mPendingRequestorsAtleastOnce[key] << " " << value << " " << valuevalue << " " << mCurrServiceSlice << " " << mNumEpochsSilent << std::endl;
				  if(mPendingRequestorsSet.size() > 1) /*have other waiting tenants*/
				  {
					  std::cout << "RESPONSE NONE->OTHERS ";
					  for(auto const& p : mPendingRequestorsSet)
					  {
						  if(p != key)
						  {
							  std::cout << p << " " << mRequestorCumulatedServiceSliceMap[p] << " " << mRequestorCurrentAllottedServiceSliceMap[p] << " ";
						  }
					  }
					  std::cout << std::endl;
					  if(mSchedulingEpoch * mNumEpochsSilent >= valuevalue)
					  {
						  mRequestorCumulatedServiceSliceMap.erase(key);
						  mRequestorCurrentAllottedServiceSliceMap.erase(key);
						  /*Find the next tenant to be serviced*/
						  auto const& s = mPendingRequestorsSet.upper_bound(mTenantBeingServiced);
						  if(s == mPendingRequestorsSet.end())
						  {
							  auto const& t = mPendingRequestorsSet.lower_bound(mTenantBeingServiced);
							  if(t == mPendingRequestorsSet.begin())
							  {
								  std::cout << "RESPONSE NONE->WAS ONLY TENANT SO -1" << std::endl;
								  mTenantBeingServiced = -1; /*set to an illegal value for error checking*/
							  }
							  else
							  {
								  std::set<int>::iterator tprime = t;
								  std::set<int>::iterator tprimeminus1 = t;
                                                                  --tprimeminus1;
								  std::set<int>::iterator tprimeplus1 = t;
                                                                  ++tprimeplus1;
								  std::cout << "RESPONSE NONE->NEXT TENANT WHICH IS NOT LESS THAN " << *tprime << " " << *tprimeminus1 << " " << *tprimeplus1 << std::endl;
								  //unsigned int nextTenant = *(tprime++);
								  unsigned int nextTenant = *(tprimeminus1);
								  mTenantBeingServiced = nextTenant;
							  }
						  }
						  else
						  {
                                                          std::cout << "RESPONSE NONE->NEXT TENANT IS GREATER THAN" << std::endl;
							  unsigned int nextTenant = *s;
							  mTenantBeingServiced = nextTenant;
						  }
						  mPendingRequestorsSet.erase(key);
						  mServicedRequestorsSet.insert(key);
						  std::cout << "RESPONSE NONE->OTHERS->CHANGE TENANT " << mTenantBeingServiced << std::endl;
					  }
					  else
					  {
						  std::cout << "RESPONSE NONE->OTHERS-> CHANGE TENANT NO " << std::endl;
					  }
				  }
                                  else
                                  {
                                      std::cout << "RESPONSE NONE->OTHERS NONE " << std::endl;
                                  }
			  }
		  }
		  else
		  {
			  //tenantBeingServiced does a handoff to a next valid tenant unless nothing pending
			  std::cout << "tenantBeingServiced does a handoff to a next valid tenant unless nothing pending "  << mTenantBeingServiced << std::endl;
			  assert(0); 
		  }
	  }
          else
          {
               //std::cout << "RESPONSE NONE->PENDING NONE" << std::endl;
          }
              
	  boost::this_thread::interruption_point(); //otherwise this thread can't be terminated
  } 
}

