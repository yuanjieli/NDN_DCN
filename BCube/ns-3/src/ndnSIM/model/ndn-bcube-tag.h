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
 * Author: Yuanjie Li <yuanjie.li@cs.ucla.edu>
 */

#ifndef NDN_BCUBE_TAG_H
#define NDN_BCUBE_TAG_H

#include "ns3/tag.h"

namespace ns3 {
namespace ndn {

/**
 * @brief Packet tag that is used to track hop count for Interest-Data pairs
 */
class BCubeTag : public Tag
{
public:
  static TypeId
  GetTypeId (void);

  /**
   * @brief Default constructor
   */
  BCubeTag () : m_nexthop (std::numeric_limits<uint32_t>::max ()) { };	

  /**
   * @brief Destructor
   */
  ~BCubeTag () { }

  void
  SetNextHop(uint8_t rhs)
  {
  	m_nexthop = rhs;
  }
  
  uint32_t
  GetNextHop() const
  {
  	return m_nexthop;
  }
  
  void
  SetPrevHop(uint8_t rhs)
  {
  	m_prevhop = rhs;
  }
  
  uint32_t
  GetPrevHop() const
  {
  	return m_prevhop;
  }

  ////////////////////////////////////////////////////////
  // from ObjectBase
  ////////////////////////////////////////////////////////
  virtual TypeId
  GetInstanceTypeId () const;
  
  ////////////////////////////////////////////////////////
  // from Tag
  ////////////////////////////////////////////////////////
  
  virtual uint32_t
  GetSerializedSize () const;

  virtual void
  Serialize (TagBuffer i) const;
  
  virtual void
  Deserialize (TagBuffer i);

  virtual void
  Print (std::ostream &os) const;
  
private:
  uint8_t m_nexthop;
  uint8_t m_prevhop;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_BCUBE_TAG_H
