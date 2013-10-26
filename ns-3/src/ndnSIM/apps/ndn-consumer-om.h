/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Yuanjie Li <yuanjie.li@cs.ucla.edu>         
 */

#ifndef NDN_CONSUMER_OM_H
#define NDN_CONSUMER_OM_H

#include "ndn-consumer.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn
 * \brief Ndn application for sending out Interest packets with rate limit controlled by AIMD principle
 */
class ConsumerOm: public Consumer
{
public: 
  static TypeId GetTypeId ();
        
  /**
   * \brief Default constructor 
   * Sets up randomizer function and packet sequence number
   */
  ConsumerOm ();
  virtual ~ConsumerOm ();

  // From NdnApp
  // virtual void
  // OnInterest (const Ptr<const Interest> &interest);
  
	/**
   * @brief handle NACK packets. Interest limit will be adjusted based on NACK
   * @param interest 
   * @param packet
   */
  virtual void
  OnNack (const Ptr<const Interest> &interest, Ptr<Packet> packet);

  virtual void
  OnContentObject (const Ptr<const ContentObject> &contentObject,
                   Ptr<Packet> payload);

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN protocol
   */
  virtual void
  ScheduleNextPacket ();
    
private:
  // void
  // UpdateMean ();

  // virtual void
  // SetPayloadSize (uint32_t payload);

  // void
  // SetDesiredRate (DataRate rate);

  // DataRate
  // GetDesiredRate () const;
  
  /**
   * \brief periodically show interest limit
   */
  void
  ShowInterestLimit();	
  
protected:
  double              m_limit;				//Interest limit (packet/s)
  double              m_initLimit;		//initial Interest limit (packet/s)
  double              m_beta;					//negative feedback
  double							m_alpha;				//positive feedback
  double              m_limitInterval;
  bool                m_firstTime;
  
  TracedCallback<Ptr<Node> /* node */, uint32_t /* appID */,
                 Time /* time */, double /*m_limit*/> m_TraceLimit;
  
  
};

} // namespace ndn
} // namespace ns3

#endif
