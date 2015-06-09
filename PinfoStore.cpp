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
   while(search != mSignatureDurationMultimap.end())
   {
     if(std::abs(search->second-pinfo.second)>10000000) //only insert if duration differs by more than 10 ms
     {
       mSignatureDurationMultimap.insert(pinfo);
     }
     search++;
   }
}
