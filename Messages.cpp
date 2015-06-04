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

#include <Messages.h>

std::ostream & operator<<(std::ostream &os, const ClientMessage &cm)
{
  os << "START\n";
  os << cm.m_start;
  os << "END\n";
  os << cm.m_end;
  std::vector<LongKernel>::const_iterator itu;
  os << "USE\n";
  for(itu=cm.m_long_kernels.begin(); itu!=cm.m_long_kernels.end(); itu++)
  {
     os << "\n" << (*itu).grid[0] << " " << (*itu).grid[1] << " " << (*itu).grid[2];
     os << "\n" << (*itu).block[0] << " " << (*itu).block[1] << " " << (*itu).block[2];
     os << "\n" << (*itu).duration;
  }
  std::vector<LongGap>::const_iterator itg;
  os << "GAP\n";
  for(itg=cm.m_long_gaps.begin(); itg!=cm.m_long_gaps.end(); itg++)
  {
     os << "\n" << (*itg).leading_grid[0] << " " << (*itg).leading_grid[1] << " " << (*itg).leading_grid[2];
     os << "\n" << (*itg).leading_block[0] << " " << (*itg).leading_block[1] << " " << (*itg).leading_block[2];
     os << "\n" << (*itg).trailing_grid[0] << " " << (*itg).trailing_grid[1] << " " << (*itg).trailing_grid[2];
     os << "\n" << (*itg).trailing_block[0] << " " << (*itg).trailing_block[1] << " " << (*itg).trailing_block[2];
     os << "\n" << (*itg).duration;
  }
  //std::cout << os.tellp() << std::endl;
  return os;
}

void save_schedule(ClientMessage &cm){
    //m_stream << this;
    //std::ostringstream oss;
    //oss << cm;
    //cm.m_stream << oss.rdbuf();
    //cout << oss.rdbuf() << endl;
    cm.m_stream << cm;
    //std::cout << cm.m_stream.tellp() << std::endl;
    //std::ofstream ofs(filename);
    //boost::archive::text_oarchive oa(ofs);
    //oa << this;
}
