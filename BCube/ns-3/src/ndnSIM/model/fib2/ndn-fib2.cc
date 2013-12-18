/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ndn-fib2.h"

#include "ns3/node.h"
#include "ns3/names.h"

namespace ns3 {
namespace ndn {

TypeId 
Fib2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::Fib2") // cheating ns3 object system
    .SetParent<Object> ()
    .SetGroupName ("Ndn")
  ;
  return tid;
}

std::ostream&
operator<< (std::ostream& os, const Fib2 &fib2)
{
  os << "Node " << Names::FindName (fib2.GetObject<Node>()) << "\n";
  os << "  Dest prefix      Interfaces(Costs)                  \n";
  os << "+-------------+--------------------------------------+\n";

  fib2.Print (os);
  return os;
}

} // namespace ndn
} // namespace ns3
