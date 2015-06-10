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

void PinfoStore::addPinfo(std::pair<struct KernelSignature, unsigned long> pinfo)
{
   //std::cout << "ks(update) : " << pinfo.first.mGridX << " " << pinfo.first.mGridY << " " << pinfo.first.mGridZ << " " << pinfo.first.mBlockX << " " << pinfo.first.mBlockY << " " << pinfo.first.mBlockZ << std::endl;
   auto search = mSignatureDurationMultimap.find(pinfo.first);
   //If key not present, insert
   if(search == mSignatureDurationMultimap.end())
   {
     //std::cout << "[Key Not Present] Adding pinfo of duration " << pinfo.second << std::endl;
     std::cout << "[PINFO - new] update of interval " << pinfo.second << std::endl;
     mSignatureDurationMultimap.insert(pinfo);
     return;
   }
   //If key present, insert if value is significantly different
   bool toInsert = true;
   //std::cout << "Iterating over values with same key" << std::endl;
   while(search != mSignatureDurationMultimap.end())
   {
     //std::cout << "duration(update, seen) " << search->second << std::endl;
     //std::cout << "Found pinfo of duration " << search->second << " " << std::abs((long)(search->second)-(long)(pinfo.second)) << std::endl;
     if(std::abs((long)(search->second)-(long)(pinfo.second))<10000000) //if a close enough value already present, do not insert
     {
       //std::cout << "Not inserting this key because already present value is " << search->second << " and looking to insert value " << pinfo.second << std::endl;
       toInsert = false;
       break;
     }
     search++;
   }
   //std::cout << "Done iterating" << std::endl;
   if(toInsert)
   {
      std::cout << "[PINFO - different] update of interval " << pinfo.second << std::endl;
      //std::cout << "[Key Present] Adding pinfo of duration " << pinfo.second << std::endl;
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
      //std::cout << "Can't find pinfo " << std::endl;
      *duration = 0; //don't have a previous duration
      return false;
   }
   //std::cout << "Found pinfo(lookup)" << std::endl;
   unsigned long max_duration = 0;
   while(search != mSignatureDurationMultimap.end())
   {
      //std::cout << "duration(lookup,seen) " << search->second << std::endl;
      if(search->second>max_duration)
      {
        std::cout << "[PINFO] found interval " << search->second << std::endl;
        max_duration = search->second;
        *duration = search->second; //return the longest duration for this signature
      }
      search++;
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
