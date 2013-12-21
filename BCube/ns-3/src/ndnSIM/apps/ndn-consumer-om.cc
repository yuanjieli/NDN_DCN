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

#include <set>
#include <list>
#include <string>

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
                   MakeDoubleAccessor (&ConsumerOm::m_limit),
                   MakeDoubleChecker<double> ())
    
    .AddAttribute ("LimitInterval", "period to show the interest limit",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerOm::m_limitInterval),
                   MakeDoubleChecker<double> ())
                   
    .AddAttribute ("DataFeedback", "Positive rate feedback when receiving DATA",
                   StringValue ("20.0"),
                   MakeDoubleAccessor (&ConsumerOm::m_alpha_max),
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
  : m_initLimit (100.0)
  , m_beta (1.1)
  , m_alpha (20.0)
  , m_alpha_max (20.0)
  , m_inited(false)
  , m_limitInterval (1.0)
  , m_firstTime (true)
  , m_data_count (0)
  , m_nack_count (0)
  , m_extra_nack_count (0)
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
                                         //&Consumer::SendPacket, this);
                                         &ConsumerOm::SendRandomPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (Seconds(1.0 / m_limit),
                                       //&Consumer::SendPacket, this);
                                       &ConsumerOm::SendRandomPacket, this);
}



///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerOm::OnContentObject (const Ptr<const ContentObject> &contentObject,
                   				 Ptr<Packet> payload)
{
  if(!m_inited)
  {
	m_alpha = m_alpha_max;
	m_inited = true;
  }
	
  Consumer::OnContentObject (contentObject, payload); // tracing inside
  //update interest limit
  if(contentObject->GetCE()!=2)	//not a local cache hit
  {
  	m_limit = m_limit + m_alpha/m_limit;	//here we choose parameter such that the convergence time is similar to TCP
  	m_alpha += 1/m_limit;
  	if(m_alpha>m_alpha_max)
  		m_alpha = m_alpha_max;
  }
  else	//local hit, send next requests immediately
  	//SendPacket();
  	SendRandomPacket();
  	
  
  m_data_count++;	
  /*if(m_sendEvent.IsRunning())
  {
  	m_sendEvent.Cancel();
  	ScheduleNextPacket ();
  }*/
    
}

void
ConsumerOm::OnExtraContentObject (const Ptr<const ContentObject> &contentObject,
                   				 Ptr<Packet> payload)
{
	//rule out nacks with different prefixes
	std::list<std::string>::const_iterator rhs = contentObject->GetName().begin();
	bool match = true;
	for(std::list<std::string>::iterator it = m_interestName.begin();
			it != m_interestName.end(); it++)
	{
		if(rhs==contentObject->GetName().end()
		|| *it!=*rhs)
		{
			match = false;
			break;
		}
		rhs++;
	}
	if(!match)
	{
		//NS_LOG_UNCOND("mismatch app="<<m_interestName<<" nack="<<interest->GetName());
		return;
	}
	
	m_data_count++;
}
                   				 

void
ConsumerOm::OnNack (const Ptr<const Interest> &interest, Ptr<Packet> packet)
{
	Consumer::OnNack (interest, packet);
		
	//rule out nacks with different prefixes
	std::list<std::string>::const_iterator rhs = interest->GetName().begin();
	bool match = true;
	for(std::list<std::string>::iterator it = m_interestName.begin();
			it != m_interestName.end(); it++)
	{
		if(rhs==interest->GetName().end()
		|| *it!=*rhs)
		{
			match = false;
			break;
		}
		rhs++;
	}
	if(!match)
	{
		//NS_LOG_UNCOND("mismatch app="<<m_interestName<<" nack="<<interest->GetName());
		return;
	}
	
	//update interest limit
	if(interest->GetNack()==Interest::NACK_GIVEUP_PIT)	//NOT NACK_CONGESTION
	//if(interest->GetNack()==Interest::NACK_CONGESTION)
	{
		if(interest->GetIntraSharing()>=100){
			m_limit = m_limit - m_beta;
			m_nack_count++;
		}
		else{
			m_limit = m_limit - m_beta*(double)(interest->GetIntraSharing())/100.0;  
			//NS_LOG_UNCOND("Rate suppression at node "<<GetNode()->GetId()<<" "<<m_limit);
			m_extra_nack_count++;
			//To suppress intra-sharing competition, we may also want to decrease alpha for local requests
			m_alpha -= (double)(interest->GetIntraSharing())/100.0;
			if(m_alpha<=0)m_alpha = 1;
		}
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
	NS_LOG_UNCOND("ConsumerOm: "<<GetNode()->GetId()<<" "<<Simulator::Now().GetSeconds()<<" "<<m_limit
								<<" interest="<<m_interest_count
								<<" data="<<m_data_count
								<<" nack="<<m_nack_count
								<<" enack="<<m_extra_nack_count);
	m_TraceLimit (GetNode(), GetId(), Simulator::Now(), m_limit);
	Simulator::Schedule (Seconds (m_limitInterval), &ConsumerOm::ShowInterestLimit, this);
	m_interest_count = 0;
	m_data_count = 0;
	m_nack_count = 0;
	m_extra_nack_count = 0;
}

void
ConsumerOm::SendRandomPacket()
{
	if (!m_active) return;
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t seq=m_rand.GetValue (); 

  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
  (*nameWithSequence) (seq);
  //

  Interest interestHeader;
  interestHeader.SetNonce               (m_rand.GetValue ());
  interestHeader.SetName                (nameWithSequence);
  interestHeader.SetInterestLifetime    (m_interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << interestHeader);
  NS_LOG_INFO ("> Interest for " << seq);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (interestHeader);
  NS_LOG_DEBUG ("Interest packet size: " << packet->GetSize ());

  NS_LOG_DEBUG ("Trying to add " << seq << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");

  /*m_seqTimeouts.insert (SeqTimeout (seq, Simulator::Now ()));
  m_seqFullDelay.insert (SeqTimeout (seq, Simulator::Now ()));

  m_seqLastDelay.erase (seq);
  m_seqLastDelay.insert (SeqTimeout (seq, Simulator::Now ()));

  m_seqRetxCounts[seq] ++;

  m_transmittedInterests (&interestHeader, this, m_face);

  m_rtt->SentSeq (SequenceNumber32 (seq), 1);

  FwHopCountTag hopCountTag;
  packet->AddPacketTag (hopCountTag);*/

  m_protocolHandler (packet);

	m_interest_count++;
  ScheduleNextPacket ();
}
} // namespace ndn
} // namespace ns3
