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

#include <Communicator.h>

Communicator::Communicator(std::string& url, Component who)
        :mURL(url)
{
    if(who == CLIENT)
    {
       assert((mSock = nn_socket(AF_SP, NN_PUSH)) >= 0);
    }
    else if(who == DAEMON)
    {
       assert((mSock = nn_socket(AF_SP, NN_PULL)) >= 0);
    }
    else
    {
       assert(0 && "Component has to be CLIENT or DAEMON");
    }
}

Communicator::~Communicator()
{
    assert(nn_shutdown(mSock, 0) >= 0);
}

int Communicator::connect()
{
    int ret = nn_connect(mSock, mURL.c_str());
    assert(ret >= 0);
    return ret;
}

int Communicator::bind()
{
    int ret = nn_bind (mSock, mURL.c_str());
    assert(ret >= 0);
    return ret;
}

int Communicator::receive(void *buf)
{
   int bytes = nn_recv(mSock, buf, NN_MSG, 0);
   assert(bytes >= 0);
   return bytes;
}

int Communicator::freemsg(void *buf)
{
   int ret = nn_freemsg(buf);
   assert(ret >= 0);
   return ret;
}

int Communicator::send(const void *buf, size_t size)
{
   int bytes = nn_send(mSock, buf, size, 0);
   assert(bytes >= 0);
   return bytes;
}
