/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
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
 * Author:  Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "nacks.h"

#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-content-store.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-bcube-tag.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-app.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/names.h"

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.fw.Nacks");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (Nacks);

TypeId
Nacks::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::Nacks")
    .SetGroupName ("Ndn")
    .SetParent<ForwardingStrategy> ()

    ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////

    .AddTraceSource ("OutNacks",  "OutNacks",  MakeTraceSourceAccessor (&Nacks::m_outNacks))
    .AddTraceSource ("InNacks",   "InNacks",   MakeTraceSourceAccessor (&Nacks::m_inNacks))
    .AddTraceSource ("DropNacks", "DropNacks", MakeTraceSourceAccessor (&Nacks::m_dropNacks))

    .AddAttribute ("EnableNACKs", "Enabling support of NACKs",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Nacks::m_nacksEnabled),
                   MakeBooleanChecker ())
    ;
  return tid;
}

void
Nacks::OnInterest (Ptr<Face> inFace,
                   Ptr<const Interest> header,
                   Ptr<const Packet> origPacket)
{
  if (header->GetNack () > 0)
    OnNack (inFace, header, origPacket/*original packet*/);
  else
    super::OnInterest (inFace, header, origPacket/*original packet*/);
}

void
Nacks::OnNack (Ptr<Face> inFace,
               Ptr<const Interest> header,
               Ptr<const Packet> origPacket)
{
  // NS_LOG_FUNCTION (inFace << header->GetName ());
  m_inNacks (header, inFace);

  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*header);
  if (pitEntry == 0)
    {
      // somebody is doing something bad
      m_dropNacks (header, inFace);
      return;
    }
   
  DidReceiveValidNack (inFace, header->GetNack (), header, origPacket, pitEntry);
}

void
Nacks::DidReceiveDuplicateInterest (Ptr<Face> inFace,
                                    Ptr<const Interest> header,
                                    Ptr<const Packet> origPacket,
                                    Ptr<pit::Entry> pitEntry)
{
  super::DidReceiveDuplicateInterest (inFace, header, origPacket, pitEntry);

  if (m_nacksEnabled)
    {
      NS_LOG_DEBUG ("Sending NACK_LOOP");
      Ptr<Interest> nackHeader = Create<Interest> (*header);
      nackHeader->SetNack (Interest::NACK_LOOP);
      Ptr<Packet> nack = Create<Packet> ();
      nack->AddHeader (*nackHeader);

      FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag))
        {
     	  //nack->AddPacketTag (hopCountTag);
        }
      else
        {
          NS_LOG_DEBUG ("No FwHopCountTag tag associated with received duplicated Interest");
        }
        
	  //FIXME: add BCubeTag here!

      inFace->Send (nack);
      m_outNacks (nackHeader, inFace);
    }
}
   
void
Nacks::DidExhaustForwardingOptions (Ptr<Face> inFace,
                                    Ptr<const Interest> header,
                                    Ptr<const Packet> origPacket,
                                    Ptr<pit::Entry> pitEntry)
{
	//NS_LOG_UNCOND(Names::FindName(inFace->GetNode())<<" generates nack");
	Ptr<fib::Entry> fibEntry=pitEntry->GetFibEntry();
	fib::FaceMetricContainer::type::index<fib::i_face>::type::iterator record;
	if (inFace!=0 && DynamicCast<AppFace>(inFace)==0)
	{
		record = fibEntry->m_faces.get<fib::i_face> ().find (inFace); 
	}
	
	
  if (m_nacksEnabled)
    {
      Ptr<Packet> packet = Create<Packet> ();
      Ptr<Interest> nackHeader = Create<Interest> (*header);
      nackHeader->SetNack (Interest::NACK_GIVEUP_PIT);
      //nackHeader->SetNack (Interest::NACK_CONGESTION);
      packet->AddHeader (*nackHeader);
	    	

      FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag))
        {
     	  	//packet->AddPacketTag (hopCountTag);
        }
      else
        {
          NS_LOG_DEBUG ("No FwHopCountTag tag associated with original Interest");
        }

			//Forward NACK to all faces in PIT
      BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
        {
          NS_LOG_DEBUG ("Send NACK for " << boost::cref (nackHeader->GetName ()) << " to " << boost::cref (*incoming.m_face));
          
          Ptr<Packet> target = packet->Copy();
    	  Ptr<Interest> NewHeader = Create<Interest> ();
    	  target->RemoveHeader(*NewHeader);
    	  //This is for faces who want the data, so SetIntraSharing = 0
    	  NewHeader->SetIntraSharing (120);	//larger than 100
    	
	                      	
	      target->AddHeader(*NewHeader);	
	        
	      BCubeTag tag;
		  target->RemovePacketTag(tag);
		  /* To simplify implementation, we make the following assumption:
	 	   * For each switch, it has no more than 10 ports (e.g. BCube[8,3] can already support 4096 switches)
	 	   * So RoutingCost = prevhop*10 + nexthop;
	       */
	      tag.SetInterest(0);	//nack is forwarded in the same way as data
		  tag.SetNextHop(incoming.m_localport);
		  target->AddPacketTag(tag);
          //incoming.m_face->Send (packet->Copy ());	//by Felix: NACK is multicasted!!!
		  NS_LOG_UNCOND(Names::FindName(inFace->GetNode())
		  			 <<" wants to send nack to "<<(uint32_t)incoming.m_localport);
		  incoming.m_face->Send(target);
					
          m_outNacks (nackHeader, incoming.m_face);
        }
        
      //copy NACK to all applications of this node
      Ptr<Node> node = inFace->GetNode();
      NS_ASSERT(node!=0);
      for(uint32_t k=0; k!=node->GetNApplications(); k++)
      {
      	Ptr<App> app = DynamicCast<App>(node->GetApplication(k));
      	if(app!=0)
      	{
      		bool ignore = false;
      		//If you already decrease the rate, don't decrease again
      		BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
      		{
      			if(app->GetFace()->GetId()==incoming.m_face->GetId()){
      				ignore = true;
      				break;
      			}
      		}
      		//if inFace is not an application face, we may have intra-sharing problem
      		if(!ignore && DynamicCast<AppFace>(inFace)==0)
      		{
      			/*if(inFace->GetNode()->GetId()<=1)
      				NS_LOG_UNCOND("Node="<<inFace->GetNode()->GetId()
      										<<" Extra NACK from face="<<inFace->GetId()
      										<<" fraction="<<100-record->GetFraction()
      										<<" to"<<app->GetFace()->GetId());*/
	      		nackHeader->SetIntraSharing(100-record->GetFraction());
	      		app->OnNack(nackHeader, origPacket->Copy());
	      	}
      	}
      	
      }
      
      

      pitEntry->ClearOutgoing (); // to force erasure of the record
    }

  super::DidExhaustForwardingOptions (inFace, header, origPacket, pitEntry);
}

void
Nacks::DidReceiveValidNack (Ptr<Face> inFace,
                            uint32_t nackCode,
                            Ptr<const Interest> header,
                            Ptr<const Packet> origPacket,
                            Ptr<pit::Entry> pitEntry)
{
  NS_LOG_DEBUG ("nackCode: " << nackCode << " for [" << header->GetName () << "]");

  // If NACK is NACK_GIVEUP_PIT, then neighbor gave up trying to and removed it's PIT entry.
  // So, if we had an incoming entry to this neighbor, then we can remove it now
  if (nackCode == Interest::NACK_GIVEUP_PIT)
    {
      pitEntry->RemoveIncoming (inFace);
    }

  if (nackCode == Interest::NACK_LOOP ||
      nackCode == Interest::NACK_CONGESTION ||
      nackCode == Interest::NACK_GIVEUP_PIT)
    {
      pitEntry->SetWaitingInVain (inFace);

      if (!pitEntry->AreAllOutgoingInVain ()) // not all ougtoing are in vain
        {
          NS_LOG_DEBUG ("Not all outgoing are in vain");
          // suppress
          // Don't do anything, we are still expecting data from some other face
          m_dropNacks (header, inFace);
          return;
        }

      Ptr<Packet> nonNackInterest = Create<Packet> ();
      Ptr<Interest> nonNackHeader = Create<Interest> (*header);
      nonNackHeader->SetNack (Interest::NORMAL_INTEREST);
      nonNackInterest->AddHeader (*nonNackHeader);

      FwHopCountTag hopCountTag;
      if (origPacket->PeekPacketTag (hopCountTag))
        {
     	  //nonNackInterest->AddPacketTag (hopCountTag);
        }
      else
        {
          NS_LOG_DEBUG ("No FwHopCountTag tag associated with received NACK");
        }

			//by Felix: try to propagate this interest to other available output interfaces
      bool propagated = DoPropagateInterest (inFace, nonNackHeader, nonNackInterest, pitEntry);
      if (!propagated)
        {
        	//by Felix: all the forwarding options are in vain. A Nack will be forwarded
          DidExhaustForwardingOptions (inFace, nonNackHeader, nonNackInterest, pitEntry);
        }
    }
}

} // namespace fw
} // namespace ndn
} // namespace ns3
