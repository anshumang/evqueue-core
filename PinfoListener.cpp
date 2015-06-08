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

#include "PinfoListener.h"

PinfoListener::PinfoListener(std::string url)
{
   std::cout << "Creating PinfoListener at " << url << std::endl;
   mComm = new Communicator(url, RECEIVER);
   mComm->bind();
}

PinfoListener::~PinfoListener()
{
   delete mComm;
}

void PinfoListener::start()
{
   std::cout << "In ProcessPinfo thread" << std::endl;
   mThread = std::thread(&PinfoListener::ProcessPinfo, this);
}

void PinfoListener::join()
{
   mThread.join();
}

void PinfoListener::ProcessPinfo()
{
   while(true)
   {
     void *buf = NULL;
     int bytes = mComm->receive(&buf);
     assert(bytes >= 0);
     std::cout << "Pinfo(bytes) : " << bytes << std::endl;
     std::cout << "No of records : " << *(int *)buf << std::endl;
     std::vector<LongKernel> vecLongKernels; 
     for(int i=0; i<*(int *)buf; i++)
     {
	//std::cout << *((uint64_t*)((char *)buf+sizeof(int))+i) << std::endl;
        LongKernel *lk = (LongKernel *)((char *)buf+sizeof(int))+i;
        LongKernel longKernel;
        longKernel.grid[0] = lk->grid[0];
        longKernel.grid[1] = lk->grid[1];
        longKernel.grid[2] = lk->grid[2];
        longKernel.block[0] = lk->block[0];
        longKernel.block[1] = lk->block[1];
        longKernel.block[2] = lk->block[2];
        longKernel.duration = lk->duration;
        vecLongKernels.push_back(longKernel);
     }
     //ProfileInfo pinfo((*(int *)buf), (int *)buf+sizeof(int));
     ProfileInfo pinfo(vecLongKernels.size(), vecLongKernels);
     pinfo.printPinfo();
   }
}
