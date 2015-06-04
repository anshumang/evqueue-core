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

#include <map>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <set>
#include "ReqRespDescriptor.h"

struct RequestWindow
{
  std::vector <std::queue <RequestDescriptor *> > mPerTenantRequestQueue;
  std::set <int> mWaitingTenantId;
  RequestWindow();
  //mPerTenantRequestQueue helpers
  void addRequest(int tenantId, RequestDescriptor *reqDesc);
  RequestDescriptor* peekRequest(int tenantId);
  //mWaitingTenantId helpers
  void addToWaiters(int tenantId);
};
