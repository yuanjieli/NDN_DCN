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

void
BCubeTag::CopyHop(uint8_t *rhs, uint8_t n_hop)
{
	NS_ASSERT(n_hop>0 && n_hop<=MAX_K);
	m_totalhop = n_hop ;
	m_cur = 0;
	memcpy(m_tags,rhs,sizeof(uint8_t)*n_hop);
}

uint32_t
BCubeTag::GetSerializedSize () const
{
  return sizeof(uint8_t)*(2+MAX_K);
}

void
BCubeTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_totalhop);
  i.WriteU8 (m_cur+1);	//point to next hop
  for(size_t j = 0; j != MAX_K; j++)
  	i.WriteU8 (m_tags[j]);
  	 
}
  
void
BCubeTag::Deserialize (TagBuffer i)
{
  m_totalhop = i.ReadU8 ();
  m_cur = i.ReadU8 ();
  for(size_t j = 0; j != MAX_K; j++)
  	m_tags[j] =  i.ReadU8 ();
  
  NS_ASSERT(m_cur>=0 && m_cur<MAX_K);	
  m_nexthop = m_tags[m_cur];
  if(m_cur==0)
  	m_prevhop = std::numeric_limits<uint32_t>::max ();
  else
  	m_prevhop = m_tags[m_cur-1];
}

void
BCubeTag::Print (std::ostream &os) const
{
  os << m_nexthop;
}

} // namespace ndn
} // namespace ns3
