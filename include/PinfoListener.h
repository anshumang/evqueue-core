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

#ifndef _PINFOLISTENER_H
#define _PINFOLISTENER_H

#include <boost/thread.hpp>
#include "Communicator.h"
#include "Messages.h"
#include "PinfoStore.h"
#include <thread>
#include <map>
#include <utility>

struct PinfoListener
{
  PinfoListener(std::string url, PinfoStore& pinfos);
  ~PinfoListener();
  void start();
  void join();
  void ProcessPinfo();
  Communicator *mComm;
  std::thread mThread;
  PinfoStore& mPinfos;
};

#endif