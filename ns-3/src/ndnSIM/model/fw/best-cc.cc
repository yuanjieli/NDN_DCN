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

#include "best-cc.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-limits-delta-rate.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <vector>
#include <cstdlib>
#include <ctime>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;
	
namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (BestCC);

LogComponent BestCC::g_log = LogComponent (BestCC::GetLogName ().c_str ());

std::string
BestCC::GetLogName ()
{
  return BestCC::super::GetLogName ()+".BestCC";
}


TypeId
BestCC::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::BestCC")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <BestCC> ()
    ;
  return tid;
}

BestCC::BestCC ()
{
	srand(time(0));
}

bool
BestCC::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,
                                Ptr<const Packet> origPacket,
                                Ptr<pit::Entry> pitEntry)
{
	  NS_LOG_FUNCTION (this << header->GetName ());
	  int propagatedCount = 0;
	  double max_M = -1;
	  Ptr<Face> optimalFace=0;
  
  
  	//Step1: filter all the faces with minimum cost
	  double minCost = 10000; //inf
	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	  	{
	  		if(metricFace.GetRoutingCost()<minCost)
	  			minCost = metricFace.GetRoutingCost();
	  	}
	  	
	  std::vector< Ptr<Face> > vecFaces;
	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	  	{
	  		if(metricFace.GetRoutingCost()==minCost)
	  			vecFaces.push_back(metricFace.GetFace ());
	  	}
	
		//Step2: choose ONE face based on our congestion control strategy
		
	  for(std::vector< Ptr<Face> >::iterator it = vecFaces.begin(); it !=vecFaces.end(); it++)
	  {
	  	if (DynamicCast<AppFace> (*it) !=0)	//this is an application
	      {
	      	optimalFace = *it;
	      	max_M = 0;
	      	break;
	      }
	    Ptr<LimitsDeltaRate> faceLimits = (*it)->GetObject<LimitsDeltaRate> ();
	    if (faceLimits->GetAvailableInterestIncrement()>max_M 
	      &&  CanSendOutInterest (inFace, *it, header, origPacket, pitEntry))
	      {
	      	optimalFace = *it;
	      	max_M = faceLimits->GetAvailableInterestIncrement();
	      }
	  }
	  
	  if (max_M == -1) //no interface available, return a NACK
	  	return false;
	  
	  Ptr<Limits> faceLimits = optimalFace->GetObject<Limits> ();
	  faceLimits->BorrowLimit ();
	  TrySendOutInterest (inFace, optimalFace, header, origPacket, pitEntry);
	  propagatedCount++;
  
  
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
  
  
}

/*void
BestCC::DidReceiveValidNack (Ptr<Face> inFace,
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
      //by Felix: We don't try alternative faces, which will introduce unfairness in multi-hop environment
      DidExhaustForwardingOptions (inFace, nonNackHeader, nonNackInterest, pitEntry);
    }
}*/

void
BestCC::OnNack (Ptr<Face> inFace,
               Ptr<const Interest> header,
               Ptr<const Packet> origPacket)
{
  super::OnNack (inFace, header, origPacket);
  Ptr<LimitsDeltaRate> faceLimits = inFace->GetObject<LimitsDeltaRate> ();
  if(faceLimits)
  	faceLimits->IncreaseNack ();
}

class PerOutFaceDeltaLimits;
NS_OBJECT_ENSURE_REGISTERED (PerOutFaceDeltaLimits);

#ifdef DOXYGEN
// /**
//  * \brief Strategy implementing per-out-face limits on top of BestCC strategy
//  */
class BestCC::PerOutFaceDeltaLimits : public ::ns3::ndn::fw::PerOutFaceDeltaLimits { };

#endif


} // namespace fw
} // namespace ndn
} // namespace ns3
