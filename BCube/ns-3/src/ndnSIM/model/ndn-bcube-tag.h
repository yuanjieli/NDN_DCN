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
#include <cmath>

#define MAX_K 10	//maximum number of hops

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
  BCubeTag () : 
    m_cur (1)	
  ,	m_metric (std::numeric_limits<uint32_t>::max ()) 
  , m_interest (0)
  { 
  };	

  /**
   * @brief Destructor
   */
  ~BCubeTag () { }
  
  void
  SetForwardingTag(uint32_t rhs)
  {
  	m_metric = rhs;
  }
  
  uint32_t
  GetForwardingTag() const
  {
  	return m_metric;
  }
  
  void
  SetCurTag(uint8_t cur)
  {
  	NS_ASSERT(cur>0 && cur<MAX_K);
  	m_cur = cur;
  }
  
  uint8_t
  GetCurTag() const
  {
  	return m_cur;
  }
  
  uint32_t
  GetNextHop() const
  {
  	return m_nexthop;
  }
  
  uint32_t
  GetPrevHop() const
  {
  	return m_prevhop;
  }
  
  void
  SetNextHop(uint32_t rhs)	//For Data packet only
  {
  	NS_ASSERT(m_interest == 0);
  	m_nexthop = rhs;
  }
  
  void
  SetPrevHop(uint32_t rhs)	//For interest packet only
  {
  	NS_ASSERT(m_interest != 0);
  	m_prevhop = rhs;
  }
  
  void
  SetInterest(uint8_t rhs)
  {
  	m_interest = rhs;
  }
  
  uint8_t
  GetInterest() const
  {
  	return m_interest;
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
  uint8_t m_cur;
  uint32_t m_metric;
  uint8_t m_interest;	//for interest or data?
  uint32_t m_nexthop;
  uint32_t m_prevhop;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_BCUBE_TAG_H
