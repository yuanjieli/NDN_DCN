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


#ifndef NDNSIM_BEST_CC_H
#define NDNSIM_BEST_CC_H

#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "nacks.h"

#include "ns3/simulator.h"
#include "ns3/string.h"

#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-limits-delta-rate.h"

#include "ns3/ndn-limits.h"


namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best congestion control strategy. This strategy assumes the existence of per-interface rate limit (ndn-limits-delta-rate)
 */
class BestCC :
    public Nacks
{
private:
  typedef Nacks super;

public:
  static TypeId
  GetTypeId ();

  /**
   * @brief Helper function to retrieve logging name for the forwarding strategy
   */
  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  BestCC ();
        
  // from super
  virtual bool
  DoPropagateInterest (Ptr<Face> incomingFace,
                       Ptr<const Interest> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry);
  virtual void
  OnNack (Ptr<Face> inFace,
          Ptr<const Interest> header,
          Ptr<const Packet> origPacket);                     	
                       	
  
  /*virtual void
  DidReceiveValidNack (Ptr<Face> inFace,
                       uint32_t nackCode,
                       Ptr<const Interest> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry); */                    	
                       	
protected:
  static LogComponent g_log;
};

//rate-based Interest limit
class PerOutFaceDeltaLimits :
    public BestCC
{
private:
  typedef BestCC super;

public:
  /**
   * @brief Get TypeId of the class
   */
  static TypeId
  GetTypeId ();

  /**
   * @brief Helper function to retrieve logging name for the forwarding strategy
   */
  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  PerOutFaceDeltaLimits ()
  { }

  /// \copydoc ForwardingStrategy::WillEraseTimedOutPendingInterest
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

  /// \copydoc ForwardingStrategy::AddFace
  virtual void
  AddFace (Ptr<Face> face)
  {
    ObjectFactory factory ("ns3::ndn::Limits::LimitsDeltaRate");
    Ptr<Limits> limits = factory.Create<Limits> ();
    face->AggregateObject (limits);

    super::AddFace (face);
  }
  
protected:
  /// \copydoc ForwardingStrategy::CanSendOutInterest
  virtual bool
  CanSendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const Interest> header,
                      Ptr<const Packet> origPacket,
                      Ptr<pit::Entry> pitEntry);
  
  /// \copydoc ForwardingStrategy::WillSatisfyPendingInterest
  virtual void
  WillSatisfyPendingInterest (Ptr<Face> inFace,
                              Ptr<pit::Entry> pitEntry);
                              
protected:
  static LogComponent g_log; ///< @brief Logging variable
};

LogComponent PerOutFaceDeltaLimits::g_log = LogComponent (PerOutFaceDeltaLimits::GetLogName ().c_str ());


std::string
PerOutFaceDeltaLimits::GetLogName ()
{
  return super::GetLogName ()+".PerOutFaceDeltaLimits";
}

TypeId
PerOutFaceDeltaLimits::GetTypeId (void)
{
  static TypeId tid = TypeId ((super::GetTypeId ().GetName ()+"::PerOutFaceDeltaLimits").c_str ())
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <PerOutFaceDeltaLimits> ()
    ;
  return tid;
}

bool
PerOutFaceDeltaLimits::CanSendOutInterest (Ptr<Face> inFace,
                                           Ptr<Face> outFace,
                                           Ptr<const Interest> header,
                                           Ptr<const Packet> origPacket,
                                           Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());
  
  Ptr<Limits> faceLimits = outFace->GetObject<Limits> ();
  if (faceLimits->IsBelowLimit ())
    {
      if (super::CanSendOutInterest (inFace, outFace, header, origPacket, pitEntry))
        {
          //faceLimits->BorrowLimit ();
          return true;
        }
    }
  
  NS_LOG_INFO("NACK "<<DynamicCast<LimitsDeltaRate> (faceLimits)->GetCurrentLimitRate()
  						<<" "<<DynamicCast<LimitsDeltaRate> (faceLimits)->GetCurrentCounter());
  return false;
}

void
PerOutFaceDeltaLimits::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());

  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      Ptr<Limits> faceLimits = face->m_face->GetObject<Limits> ();
      faceLimits->ReturnLimit ();
    }

  super::WillEraseTimedOutPendingInterest (pitEntry);
}

void
PerOutFaceDeltaLimits::WillSatisfyPendingInterest (Ptr<Face> inFace,
                                                   Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());

  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      Ptr<Limits> faceLimits = face->m_face->GetObject<Limits> ();
      faceLimits->ReturnLimit ();
    }
  
  super::WillSatisfyPendingInterest (inFace, pitEntry);
}

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_BEST_CC_H
