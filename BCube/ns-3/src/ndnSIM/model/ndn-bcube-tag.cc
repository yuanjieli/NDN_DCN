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
  return sizeof(uint32_t);
}

void
BCubeTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_nexthop);
}
  
void
BCubeTag::Deserialize (TagBuffer i)
{
  m_nexthop = i.ReadU32 ();
}

void
BCubeTag::Print (std::ostream &os) const
{
  os << m_nexthop;
}

} // namespace ndn
} // namespace ns3
