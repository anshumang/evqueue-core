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

#ifndef _PINFOSTORE_H
#define _PINFOSTORE_H

#include "Messages.h"
#include <map>
#include <utility>

struct KernelSignature
{
   unsigned long mGridX;
   unsigned long mGridY;
   unsigned long mGridZ;
   unsigned long mBlockX;
   unsigned long mBlockY;
   unsigned long mBlockZ;
};

struct CompareKernelSignature
{
  bool operator()(KernelSignature ks1, KernelSignature ks2)
  {
     return ks1.mGridX*ks1.mGridY*ks1.mGridZ < ks2.mGridX*ks2.mGridY*ks2.mGridZ;
  }
};

struct PinfoStore
{
  void addPinfo(std::pair<KernelSignature, unsigned long>);
  std::multimap<KernelSignature, unsigned long, CompareKernelSignature> mSignatureDurationMultimap;
};

#endif
