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

#ifndef _REQRESP_DESCRIPTOR_H
#define _REQRESP_DESCRIPTOR_H

#include <string>

using namespace std;

struct KernelIdentifier
{
   unsigned long m_grid[3];
   unsigned long m_block[3];
   string m_name;
   KernelIdentifier(string name, unsigned long grid[], unsigned long block[]);
};

struct RequestDescriptor
{
   struct KernelIdentifier kid;
   long request_timestamp;
};

struct ResponseDescriptor
{
   bool mNeedYield;
   long mRunSlice; //how long before yielding
};

#endif
