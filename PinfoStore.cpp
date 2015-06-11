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

#include "PinfoStore.h"
#include <cstdlib>

void PinfoStore::lock()
{
   mMutex.lock();
}

void PinfoStore::unlock()
{
   mMutex.unlock();
}

void PinfoStore::addPinfo(std::pair<struct KernelSignature, unsigned long> pinfo)
{
   //std::cout << "[PINFO] signature " << pinfo.first.mGridX << " " << pinfo.first.mGridY << " " << pinfo.first.mGridZ << " " << pinfo.first.mBlockX << " " << pinfo.first.mBlockY << " " << pinfo.first.mBlockZ << " of interval " << pinfo.second << std::endl;
   auto search = mSignatureDurationMultimap.find(pinfo.first);
   //If key not present, insert
   if(search == mSignatureDurationMultimap.end())
   {
     //std::cout << "[PINFO - new] update of interval " << pinfo.second << " with signature " << pinfo.first.mGridX << " " << pinfo.first.mGridY << " " << pinfo.first.mGridZ << " " << pinfo.first.mBlockX << " " << pinfo.first.mBlockY << " " << pinfo.first.mBlockZ << std::endl;
     mSignatureDurationMultimap.insert(pinfo);
     return;
   }
   //If key present, insert if value is significantly different
   bool toInsert = true;
   auto range = mSignatureDurationMultimap.equal_range(pinfo.first);
   //while(search != mSignatureDurationMultimap.end())
   for(auto iterator = range.first; iterator != range.second; iterator++)
   {
     //std::cout << "duration(update, seen) " << iterator->second << std::endl;
     if(std::abs((long)(iterator->second)-(long)(pinfo.second))<5000000) //if a close enough value (<scheduling epoch/2) already present, do not insert
     {
       toInsert = false;
       break;
     }
     //search++;
   }
   if(toInsert)
   {
     //std::cout << "[PINFO - different] update of interval " << pinfo.second << " with signature " << pinfo.first.mGridX << " " << pinfo.first.mGridY << " " << pinfo.first.mGridZ << " " << pinfo.first.mBlockX << " " << pinfo.first.mBlockY << " " << pinfo.first.mBlockZ << std::endl;
      mSignatureDurationMultimap.insert(pinfo);
   }
}

bool PinfoStore::hasPinfo(struct KernelSignature ks, unsigned long *duration)
{
   //std::cout << "ks(lookup) : " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " " << ks.mBlockX << " " << ks.mBlockY << " " << ks.mBlockZ << std::endl;
   auto search = mSignatureDurationMultimap.find(ks);
   //showPinfoStore();
   if(search == mSignatureDurationMultimap.end())
   {
      *duration = 0; //don't have a previous duration
      return false;
   }
   //std::cout << "Found pinfo(lookup)" << std::endl;
   auto range = mSignatureDurationMultimap.equal_range(ks);
   unsigned long max_duration = 0;
   //while(search != mSignatureDurationMultimap.end())
   for(auto iterator = range.first; iterator != range.second; iterator++)
   {
      //std::cout << "duration(lookup,seen) " << search->second << std::endl;
      if(iterator->second>max_duration)
      {
        //std::cout << "[PINFO] found interval " << search->second << std::endl;
        max_duration = iterator->second;
        *duration = iterator->second; //return the longest duration for this signature
      }
      //search++;
   }
   return true;
}

void PinfoStore::showPinfoStore()
{
   for(auto const &p : mSignatureDurationMultimap)
   {
        KernelSignature ks = p.first;
	std::cout << "showPinfoStore : " << ks.mGridX << " " << ks.mGridY << " " << ks.mGridZ << " " << ks.mBlockX << " " << ks.mBlockY << " " << ks.mBlockZ << " => ";
	std::cout << p.second << " ";
        std::cout << std::endl;
   }
}
