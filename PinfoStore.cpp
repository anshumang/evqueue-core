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

void PinfoStore::addPinfo(std::pair<KernelSignature, unsigned long> pinfo)
{
   auto search = mSignatureDurationMultimap.find(pinfo.first);
   //If key not present, insert
   if(search == mSignatureDurationMultimap.end())
   {
     std::cout << "[Key Not Present] Adding pinfo of duration " << pinfo.second << std::endl;
     mSignatureDurationMultimap.insert(pinfo);
     return;
   }
   //If key present, insert if value is significantly different
   bool toInsert = true;
   std::cout << "Iterating over values with same key" << std::endl;
   while(search != mSignatureDurationMultimap.end())
   {
     std::cout << "Found pinfo of duration " << search->second << " " << std::abs((long)(search->second)-(long)(pinfo.second)) << std::endl;
     if(std::abs((long)(search->second)-(long)(pinfo.second))<10000000) //if a close enough value already present, do not insert
     {
       std::cout << "Not inserting this key because already present value is " << search->second << " and looking to insert value " << pinfo.second << std::endl;
       toInsert = false;
       break;
     }
     search++;
   }
   std::cout << "Done iterating" << std::endl;
   if(toInsert)
   {
      std::cout << "[Key Present] Adding pinfo of duration " << pinfo.second << std::endl;
      mSignatureDurationMultimap.insert(pinfo);
   }
}
