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
  unsigned long last_max_duration = 0, last_service_slice = 0;
  int last_pending_requestor_serviced=-1, last_pending_requestors_cnt=-1, epochs_since_last_response=0, num_epochs_standalone=0;
  while(true) //runs forever
  {
	  boost::this_thread::sleep(epoch);
          epochs_since_last_response++;
	  last_pending_requestors_cnt = mPendingRequestorsSet.size();
	  //queue gets processed here
          auto tenantId = 0;
          std::vector<std::pair<unsigned long, int>> deadlinesPerTenant;
          
          for (auto const& p : mReqWindow->mPerTenantRequestQueue)
          {
            mReqWindow->lock();
            RequestDescriptor *reqDesc = mReqWindow->consumeRequest(tenantId);
            mReqWindow->unlock();
            if(reqDesc)
            {
                std::pair<unsigned long, int> q;
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
                    std::cout << "progress for " << tenantId << " " << " with request of " << tot_run_time << " " << have_run_for << std::endl;
                    //std::cout << "Yield-ed kernel " << tot_run_time << " " << have_run_for << std::endl;
                    if(tot_run_time +10000000 < have_run_for) /*need a tolerance bias for the predictor not being updated*/
                    {
                        //std::cout << "Prediction was wrong but can't continue " << tenantId << " " << tot_run_time << " " << have_run_for << std::endl;
                        //assert(0);
                    }
                    if(tot_run_time < have_run_for)
                    {
                        //std::cout << "Prediction was wrong but can continue " << tenantId << " " << tot_run_time << " " << have_run_for << std::endl;
                    }
                    //assert(tot_run_time >= have_run_for);
                    q = std::make_pair(tot_run_time - have_run_for, tenantId);
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
				if(mTieBreaker[0]||mTieBreaker[1]||mTieBreaker[2]||mTieBreaker[3]||mTieBreaker[4]||mTieBreaker[5])
				{
					//std::cout << "------" << std::endl;
				}
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
				if(tenantId != 0)
				{
					if(m_request_ctr[tenantId]%52>1)
					{
						/*if((last_max_duration!=0) && (max_duration - last_max_duration > 20000000))
						{
							max_duration = last_max_duration;
						}*/
						q = std::make_pair(max_duration, tenantId);
						std::cerr << tenantId << " " << m_request_ctr[tenantId] << " max " << ks.mGridX << " " << max_duration << std::endl;
						mReqWindow->setServiceId(max_duration, tenantId);
						last_max_duration = max_duration;
					}
					else
					{
						q = std::make_pair(min_duration, tenantId);
						std::cerr << tenantId << " " << m_request_ctr[tenantId] << " min " << ks.mGridX << " " << min_duration << std::endl;
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
                //std::cout << "[REQUEST] " << q.first << " " << q.second; 
            }
            else
            {
                 //std::cout << "[ARBITER] No request found from " << tenantId << std::endl;
            }
            tenantId++;
          }
          if(deadlinesPerTenant.size()>0)
          {
             //std::cout << "REQUEST A" << std::endl;
          }
          else
          {
             //std::cout << "REQUEST NA" << std::endl;
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
		  assert(mRequestorCurrentAllottedServiceSliceMap.size() == 0);
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
                       //std::cout << p.first << " " << p.second;
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
                   mRequestorCurrentAllottedServiceSliceMap.insert(mBlockedRequestorCumulatedServiceSliceMap.begin(), mBlockedRequestorCumulatedServiceSliceMap.end());
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
                                  auto rr = std::make_pair(p.second, p.first);
                                  mRequestorCurrentAllottedServiceSliceMap.insert(rr);
			  }
			  else /*pending requestor*/
			  {
                                  //std::cout << "In service Requestor " << p.second << " " << p.first << " --- ";
				  auto r = std::make_pair(p.second, p.first);
                                  auto const& q = mRequestorCumulatedServiceSliceMap.find(p.second);
                                  auto const& qq = mRequestorCurrentAllottedServiceSliceMap.find(p.second);
				  if(q != mRequestorCumulatedServiceSliceMap.end())
				  {
                                          assert(qq != mRequestorCurrentAllottedServiceSliceMap.end());
					  unsigned long updated_cumul_service_slice = (*q).second + p.first;
					  //std::cout << "In service Requestor (update) " << p.second << " " << (*q).second << " " << updated_cumul_service_slice << " --- ";
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
                  if((mPendingRequestorsSet.size()==1)&&(mTenantBeingServiced>0)) /*pr is the only tenant*/
                  {
                       //std::cout << "pr only tenant for " << ++num_epochs_standalone << std::endl;
                  }
                  if(mPendingRequestorsSet.size()==2)
                  {
                        num_epochs_standalone=0;
                        std::cout << "pr only tenant for " << last_service_slice << " " << epochs_since_last_response << " " << mSchedulingEpoch << " " << last_pending_requestor_serviced  << " " << last_pending_requestors_cnt << std::endl;
                  }
		  assert(mTenantBeingServiced != -1);
		  auto const& r = mRequestorCumulatedServiceSliceMap.find(mTenantBeingServiced);
		  auto const& rr = mRequestorCurrentAllottedServiceSliceMap.find(mTenantBeingServiced);
		  if (r != mRequestorCumulatedServiceSliceMap.end())
		  {
                          if((mPendingRequestorsSet.size()>1)&&(last_pending_requestors_cnt==1))
                          {
                             if(last_pending_requestor_serviced>0)
                             {
                             if(last_service_slice > epochs_since_last_response*mSchedulingEpoch)
                             {
                             std::cout << "nnf coming in short by  " << last_service_slice-(epochs_since_last_response*mSchedulingEpoch) << " " << last_service_slice << " " << epochs_since_last_response*mSchedulingEpoch << std::endl;
                             }
                             }
                             else
                             {
                             if(last_service_slice > epochs_since_last_response*mSchedulingEpoch)
                             {
                             std::cout << "pr coming in short by  " << last_service_slice-(epochs_since_last_response*mSchedulingEpoch) << " " << last_service_slice << " " << epochs_since_last_response*mSchedulingEpoch << std::endl;
                             }
                             }
                          }
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
                                  if(mPendingRequestorsSet.size()>1)
                                  {
                                    std::cout << "[RESPONSE Y double] " << key << " 0 " << mRequestorCumulatedServiceSliceMap[0] << " " << mRequestorCurrentAllottedServiceSliceMap[0] << " 1 " << mRequestorCumulatedServiceSliceMap[1] << " " << mRequestorCurrentAllottedServiceSliceMap[1] << " " << mCurrServiceSlice << std::endl; 
                                  }
                                  else
                                  {
                                      //std::cout << "[RESPONSE Y single] " << key << " " << valuevalue << " " << value << " " <<  mCurrServiceSlice << std::endl;
                                  }
                                  if(mPendingRequestorsAtleastOnce[key])
                                  {
                                      mPendingRequestorsAtleastOnce[key] = false;
                                  }
				  //std::cout << "Now servicing " << key << " " << value << std::endl;
				  mReqWindow->sendResponse(key);
                                  last_pending_requestor_serviced=key;
                                  last_service_slice = valuevalue;
                                  epochs_since_last_response = 0;
                                  //std::cout << key << std::endl;
                                  if((value > mCurrServiceSlice)&&(mPendingRequestorsSet.size() > 1))
                                  {
                                      if(key > 0)
                                      {
                                      std::cout << "nnf going out short by " << value-mCurrServiceSlice << " " << value << " " << mCurrServiceSlice << std::endl; 
                                      }
                                      else
                                      {
                                      std::cout << "pr going out short by " << value-mCurrServiceSlice << " " << value << " " << mCurrServiceSlice << std::endl; 
                                      }
                                  }
				  if(value >= mCurrServiceSlice) /*cumulServiceSlice higher than any pending requestor*/
				  {
					  //std::cout << "Done " << key << " with slice of " << mCurrServiceSlice << std::endl;
					  mRequestorCumulatedServiceSliceMap.erase(key);
					  mRequestorCurrentAllottedServiceSliceMap.erase(key);
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
                                  if(mPendingRequestorsSet.size()>1)
                                  {
                                    std::cout << "[RESPONSE N double] 0 " << mRequestorCumulatedServiceSliceMap[0] << " " << mRequestorCurrentAllottedServiceSliceMap[0] << " 1 " << mRequestorCumulatedServiceSliceMap[1] << " " << mRequestorCurrentAllottedServiceSliceMap[1] << mCurrServiceSlice << std::endl; 
                                  }
                                  else
                                  {
                                      std::cout << "[RESPONSE N single] " << key << " " << valuevalue << " " << value << " " <<  mCurrServiceSlice << std::endl;
                                  }
				  //can't wait forever to ensure fairness, not a lot more than the last requested duration accumulated (we don't store this separately, only the cumulative sum in mRequestorCumulatedServiceSliceMap), probably the upper bound is the cumulative sum - epoch if all requests came in a burst (-epoch is atleast the time it got since it was scheduled)
				  //let's count how many epochs we wait for the next request from this tenant
				  mNumEpochsSilent++;
                                  if(mPendingRequestorsSet.size() > 1) /*have other waiting tenants*/
                                  {
                                  if(mSchedulingEpoch * mNumEpochsSilent >= valuevalue)
                                  {
                                       std::cout << "Done waiting " << valuevalue << std::endl;
					  mRequestorCumulatedServiceSliceMap.erase(key);
					  mRequestorCurrentAllottedServiceSliceMap.erase(key);
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
				  //std::cout << "Silent for " << key << " with accumulated " << value << " and this is the " << mNumEpochsSilent << "th allowance when others waiting are ";
                                  std::cout << "Waiting but don't want to " << key << " " << value << " " << mNumEpochsSilent << "/" << " ";
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
                                      std::cout << "Waiting but don't care " << key << " " << value << " " << mNumEpochsSilent << std::endl;
                                  }
			  }
		  }
		  else
		  {
			  //tenantBeingServiced does a handoff to a next valid tenant unless nothing pending
			  assert(0); 
			  //std::cout << "tenantBeingServiced does a handoff to a next valid tenant unless nothing pending "  << std::endl;
		  }
	  }
          else
          {
               //std::cout << "RESPONSE NA" << std::endl;
          }
              
	  boost::this_thread::interruption_point(); //otherwise this thread can't be terminated
  } 
}

