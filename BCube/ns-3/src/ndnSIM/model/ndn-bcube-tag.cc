/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013 University of California, Los Angeles
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

#include "ndn-bcube-tag.h"

#include <cstdlib>
#include <cstring>

namespace ns3 {
namespace ndn {

TypeId
BCubeTag::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::ndn::BCubeTag")
    .SetParent<Tag>()
    .AddConstructor<BCubeTag>()
    ;
  return tid;
}

TypeId
BCubeTag::GetInstanceTypeId () const
{
  return BCubeTag::GetTypeId ();
}

uint32_t
BCubeTag::GetSerializedSize () const
{
  return sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)*2;
}

void
BCubeTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_metric);
  i.WriteU8 (m_cur);	
  i.WriteU8 (m_interest);
  i.WriteU32 (m_nexthop);	//for Data only
  i.WriteU32 (m_prevhop);	//for interest only
  
}
  
void
BCubeTag::Deserialize (TagBuffer i)
{
  m_metric = i.ReadU32 ();
  m_cur = i.ReadU8 (); 
  //NS_ASSERT(m_cur>=0 && m_cur<MAX_K);
  m_interest = i.ReadU8();
  m_nexthop = i.ReadU32 ();
  m_prevhop = i.ReadU32 ();
  
  //set nexthop if it is data packet
  if(m_interest)
  {
  	m_nexthop = m_metric/pow(10,m_cur);
  	m_nexthop = m_nexthop %10;
  }
  
}

void
BCubeTag::Print (std::ostream &os) const
{
  os << m_metric<<" "<<m_cur;
}

} // namespace ndn
} // namespace ns3
