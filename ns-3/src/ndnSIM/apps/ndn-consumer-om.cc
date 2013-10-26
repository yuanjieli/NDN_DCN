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

#include "ndn-consumer-om.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerOm");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (ConsumerOm);
    
TypeId
ConsumerOm::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerOm")
    .SetGroupName ("Ndn")
    .SetParent<Consumer> ()
    .AddConstructor<ConsumerOm> ()

    .AddAttribute ("InitLimit", "Initial Interest rate limit (packet/s)",
                   StringValue ("10.0"),
                   MakeDoubleAccessor (&ConsumerOm::m_initLimit),
                   MakeDoubleChecker<double> ())
    
    .AddAttribute ("LimitInterval", "period to show the interest limit",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerOm::m_limitInterval),
                   MakeDoubleChecker<double> ())
                   
    .AddAttribute ("DataFeedback", "Positive rate feedback when receiving DATA",
                   StringValue ("20.0"),
                   MakeDoubleAccessor (&ConsumerOm::m_alpha),
                   MakeDoubleChecker<double> ())
    
    .AddAttribute ("NackFeedback", "Negative rate feedback when receiving NACK",
                   StringValue ("1.1"),
                   MakeDoubleAccessor (&ConsumerOm::m_beta),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("MaxSeq",
                   "Maximum sequence number to request",
                   IntegerValue (std::numeric_limits<uint32_t>::max ()),
                   MakeIntegerAccessor (&ConsumerOm::m_seqMax),
                   MakeIntegerChecker<uint32_t> ())
                   
    .AddTraceSource ("TraceLimit", "Current rate limit",
                     MakeTraceSourceAccessor (&ConsumerOm::m_TraceLimit))
      

    ;

  return tid;
}
    
ConsumerOm::ConsumerOm ()
  : m_initLimit (10.0)
  , m_beta (1.1)
  , m_alpha (20.0)
  , m_limitInterval (1.0)
  , m_firstTime (true)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_seqMax = std::numeric_limits<uint32_t>::max ();
  m_limit = m_initLimit;
  Simulator::Schedule (Seconds (m_limitInterval), &ConsumerOm::ShowInterestLimit, this);
}

ConsumerOm::~ConsumerOm ()
{
}

void
ConsumerOm::ScheduleNextPacket ()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &Consumer::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (Seconds(1.0 / m_limit),
                                       &Consumer::SendPacket, this);
}



///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerOm::OnContentObject (const Ptr<const ContentObject> &contentObject,
                   				 Ptr<Packet> payload)
{
  Consumer::OnContentObject (contentObject, payload); // tracing inside
  //update interest limit
  m_limit = m_limit + m_alpha/m_limit;	//here we choose parameter such that the convergence time is similar to TCP
  
  /*if(m_sendEvent.IsRunning())
  {
  	m_sendEvent.Cancel();
  	ScheduleNextPacket ();
  }*/
    
}

void
ConsumerOm::OnNack (const Ptr<const Interest> &interest, Ptr<Packet> packet)
{
	Consumer::OnNack (interest, packet);
	//update interest limit
	if(interest->GetNack()==Interest::NACK_GIVEUP_PIT)	//NOT NACK_CONGESTION
	{
		m_limit = m_limit - m_beta;
		if (m_limit <= m_initLimit)		//we need to avoid non-sense interest limit
			m_limit = m_initLimit;	
			
		/*if(m_sendEvent.IsRunning())
	  {
	  	m_sendEvent.Cancel();
	  	ScheduleNextPacket ();
	  }*/
		
	}
	NS_LOG_INFO(GetNode()->GetId()<<" receives  nack");
	
}

void
ConsumerOm::ShowInterestLimit()
{
	NS_LOG_UNCOND(GetNode()->GetId()<<" "<<Simulator::Now().GetSeconds()<<" "<<m_limit);
	m_TraceLimit (GetNode(), GetId(), Simulator::Now(), m_limit);

	Simulator::Schedule (Seconds (m_limitInterval), &ConsumerOm::ShowInterestLimit, this);
}

} // namespace ndn
} // namespace ns3
