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

#include <ReqRespDescriptor.h>

KernelIdentifier::KernelIdentifier(string name, unsigned long grid[], unsigned long block[])
{
  m_name = name;
  m_grid[0]=grid[0];m_grid[1]=grid[1];m_grid[2]=grid[2];
  m_block[0]=block[0];m_block[1]=block[1];m_block[2]=block[2];
}

void printReqDescriptor(RequestDescriptor * reqDesc)
{
   std::cout << 
   " " << reqDesc->timestamp <<
   " " << reqDesc->grid[0] <<
   " " << reqDesc->grid[1] <<
   " " << reqDesc->grid[2] <<
   " " << reqDesc->block[0] <<
   " " << reqDesc->block[1] <<
   " " << reqDesc->block[2];
}
