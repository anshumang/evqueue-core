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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <vector>

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <sstream>

using namespace std;

struct LongKernel
{
   uint64_t grid[3];
   uint64_t block[3];
   uint64_t duration;
};

struct LongGap
{
   uint64_t leading_grid[3];
   uint64_t leading_block[3];
   uint64_t trailing_grid[3];
   uint64_t trailing_block[3];
   uint64_t duration;
};

class ClientMessage
{
    public:
       uint64_t m_start;
       uint64_t m_end;
       std::vector<LongKernel> m_long_kernels;
       std::vector<LongGap> m_long_gaps;
       std::ostringstream m_stream;

       friend std::ostream & operator<<(std::ostream &os, const ClientMessage &cm);
       friend void save_schedule(ClientMessage &cm);
       friend class boost::serialization::access;
       template<class Archive> void serialize(Archive & ar)
       {
         ar & m_long_kernels & m_long_gaps;
       }
       //void save_schedule();
       //void save_schedule(const char *filename);
       //friend void save_schedule(ClientMessage &cm);
};

void save_schedule(ClientMessage &cm);

/*std::ostream & operator<<(std::ostream &os, const ClientMessage &cm)
{

}*/

#endif

