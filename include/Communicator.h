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

#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#include <iostream>
#include <string>
#include <cassert>
#include <nn.h>
#include <pipeline.h>

using namespace std;

enum Component
{
    CLIENT,
    DAEMON
};

struct Communicator
{
    int mSock;
    std::string mURL;
    Communicator(std::string& url, Component who);
    ~Communicator();
    int connect();
    int bind();
    int send(const void *, size_t);
    int receive(void *);
    int freemsg(void *);
};

#endif
