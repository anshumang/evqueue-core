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

#include <Interposer.h>
#include <Communicator.h>
#include <chrono>
#include <EvqueueManager.h>

/*cudaError_t*/int Interposer::launch(KernelIdentifier kid) 
{
   //cudaError_t (*cudaLaunchHandle)(const void *);
   //cudaLaunchHandle = (cudaError_t (*)(const void *))dlsym(m_cudart, "cudaLaunch");
   //return cudaLaunchHandle(entry);
   std::cout << "Launch : " << kid.m_name 
   << kid.m_grid[0] << " " << kid.m_grid[1] << " " << kid.m_grid[2] << " "
   << kid.m_block[0] << " " << kid.m_block[1] << " " << kid.m_block[2] << " "
   //<< std::chrono::high_resolution_clock::now()
   << std::endl;
   RequestDescriptor reqDesc;
   reqDesc.grid[0]=kid.m_grid[0];
   reqDesc.grid[1]=kid.m_grid[1];
   reqDesc.grid[2]=kid.m_grid[2];
   reqDesc.block[0]=kid.m_block[0];
   reqDesc.block[1]=kid.m_block[1];
   reqDesc.block[2]=kid.m_block[2];
   //reqDesc.timestamp = std::chrono::high_resolution_clock::now();
   struct timeval now;
   gettimeofday(&now, NULL);
   reqDesc.timestamp = now.tv_sec * 1000000 + now.tv_usec;
   void *sendbuf = new char[sizeof(RequestDescriptor)+kid.m_name.length()+1];
   std::memcpy(sendbuf, reinterpret_cast<void*>(&reqDesc), sizeof(RequestDescriptor));
   std::memcpy(sendbuf+sizeof(RequestDescriptor), kid.m_name.c_str(), kid.m_name.length());
   char nullch = '\0';
   std::memcpy(sendbuf+sizeof(RequestDescriptor)+kid.m_name.length(), &nullch, sizeof(char));
   std::cout << "Sending bytes : " << sizeof(RequestDescriptor)+kid.m_name.length()+1 << std::endl;
   gEvqm->mReq->send(sendbuf, sizeof(RequestDescriptor)+kid.m_name.length()+1);
   return 0;
}

Interposer::Interposer()
{
   std::cout << "Interposer CTOR" << std::endl;
   //m_cudart = dlopen("/usr/local/cuda-6.5/lib64/libcudart.so", RTLD_LAZY);
   //assert(m_cudart);
}

Interposer::~Interposer()
{
   //dlclose(m_cudart);
}
