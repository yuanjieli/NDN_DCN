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

//Dynamic Forwarding
/*bool
BestCC::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,
                                Ptr<const Packet> origPacket,
                                Ptr<pit::Entry> pitEntry)
{
	  NS_LOG_FUNCTION (this << header->GetName ());
	  int propagatedCount = 0;
	  double max_M = -100000;
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
		std::vector< Ptr<Face> > OptimalCandidates;
	  for(std::vector< Ptr<Face> >::iterator it = vecFaces.begin(); it !=vecFaces.end(); it++)
	  {
	  	if (DynamicCast<AppFace> (*it) !=0)	//this is an application
	      {
	      	OptimalCandidates.push_back(*it);
	      	max_M = 0;
	      	break;
	      }
	   
	    fib::FaceMetricContainer::type::index<fib::i_face>::type::iterator record
      = pitEntry->GetFibEntry()->m_faces.get<fib::i_face> ().find (*it);
      if (record->GetSharingMetric()>=max_M 
	      &&  CanSendOutInterest (inFace, *it, header, origPacket, pitEntry))
	      {
	      	if(record->GetSharingMetric()>max_M)	
	      	{
	      		OptimalCandidates.clear();
	      	}
	      	OptimalCandidates.push_back(*it); //we wanna evenly distribute traffic over paths with equal maximum value
	      	max_M = record->GetSharingMetric();
	      }		
      
	  }
	  
	  if (max_M == -100000) //no interface available, return a NACK
	  	return false;
	  	
	  NS_ASSERT(OptimalCandidates.size()!=0);
	  
	  optimalFace = OptimalCandidates.at(rand()%OptimalCandidates.size());
	  
	  Ptr<Limits> faceLimits = optimalFace->GetObject<Limits> ();
	  faceLimits->BorrowLimit ();
	  TrySendOutInterest (inFace, optimalFace, header, origPacket, pitEntry);
	  propagatedCount++;
  
  
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
  
  
}*/

//Static Forwarding
bool
BestCC::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,
                                Ptr<const Packet> origPacket,
                                Ptr<pit::Entry> pitEntry)
{
	  NS_LOG_FUNCTION (this << header->GetName ());
	  int propagatedCount = 0;
	  Ptr<Face> optimalFace=0;
  
  
  	//Step1: filter all the faces with minimum cost
	  double minCost = 10000; //inf
	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	  	{
	  		if(metricFace.GetRoutingCost()<minCost)
	  			minCost = metricFace.GetRoutingCost();
	  	}
	  
	  double totalweight = 0;
	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	  {
	  	if(metricFace.GetRoutingCost()==minCost
	  	&& metricFace.GetFace()!=inFace)	//it happens when using non-shortest path
	  		totalweight += metricFace.GetFraction();
	  }
	  if(totalweight==0)totalweight=100;
	  double target = rand()%(int)totalweight;
	  double coin = 0;	
	  //Step2: choose ONE face based on our congestion control strategy
	  std::vector< Ptr<Face> > vecFaces;
	  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
	  	{
	  		if(metricFace.GetFace()==inFace)continue;
	  		if (DynamicCast<AppFace> (metricFace.GetFace ()) !=0)	//app-face
	  		{
	  			optimalFace = metricFace.GetFace();
	  			break;	
	  		}
	  		if(metricFace.GetRoutingCost()==minCost)
	  		{
	  			coin += metricFace.GetFraction();
	  			//if this link is already a bottleneck link, increase NACK by 1
	  			
	  			//if(coin>=target && CanSendOutInterest (inFace, metricFace.GetFace(), header, origPacket, pitEntry))
	  			//at this stage, we don't care whether this face can send interests
	  			//This would fix the bug when local link becomes bottleneck
	  			if(coin>=target)
	  			{
	  				optimalFace = metricFace.GetFace();
	  				break;
	  			}
	  		}
	  	}
	  
	  
	  if(optimalFace==0)return false;
	  
	  //If we cannot send interest through optimalFace, increase NACK
	  if(!CanSendOutInterest (inFace, optimalFace, header, origPacket, pitEntry))
	  {
	  	//we found a face, but it cannot send
	  	fib::FaceMetricContainer::type::index<fib::i_face>::type::iterator record
	   	= pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ().find (optimalFace);
  	  if (record != pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ().end ())
      {
      		
        	pitEntry->GetFibEntry ()->m_faces.modify (record,
                      ll::bind (&fib::FaceMetric::IncreaseNack, ll::_1));
      }
	  	return false;
	  }	
	  
	  Ptr<Limits> faceLimits = optimalFace->GetObject<Limits> ();
	  faceLimits->BorrowLimit ();
	  TrySendOutInterest (inFace, optimalFace, header, origPacket, pitEntry);
	  propagatedCount++;
  
  
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
  
  
}

/*void
BestCC::OnNack (Ptr<Face> inFace,
               Ptr<const Interest> header,
               Ptr<const Packet> origPacket)
{
  super::OnNack (inFace, header, origPacket);
  Ptr<LimitsDeltaRate> faceLimits = inFace->GetObject<LimitsDeltaRate> ();
  if(faceLimits)
  	faceLimits->IncreaseNack ();
}*/

//DON'T DO REROUTING
//Copy NACK to all applications of this node
//inFace is the incoming face for original interest, NOT the incoming face for nack
void
BestCC::OnNack (Ptr<Face> inFace,
               Ptr<const Interest> header,
               Ptr<const Packet> origPacket)
{
	
  //super::OnNack (inFace, header, origPacket);
  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*header);
  if (pitEntry == 0)
  {
      // somebody is doing something bad
      m_dropNacks (header, inFace);
      return;
  }
  uint32_t nackCode = header->GetNack ();
  if (nackCode == Interest::NACK_GIVEUP_PIT)
    {
      pitEntry->RemoveIncoming (inFace);
    }
    
  if (nackCode == Interest::NACK_LOOP				||
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

      //by Felix: all the forwarding options are in vain. A Nack will be forwarded
      //In DidExhaustForwardingOptions(), we will copy NACK to all applications too
      DidExhaustForwardingOptions (inFace, nonNackHeader, nonNackInterest, pitEntry);
    }
      
  Ptr<LimitsDeltaRate> faceLimits = inFace->GetObject<LimitsDeltaRate> ();
  if(faceLimits)
  	faceLimits->IncreaseNack ();
  
  //update per-fib nack counter	
  if(nackCode == Interest::NACK_GIVEUP_PIT)
  {
  	fib::FaceMetricContainer::type::index<fib::i_face>::type::iterator record
	   = pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ().find (inFace);
  	  if (record != pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ().end ())
      {
      		
        	pitEntry->GetFibEntry ()->m_faces.modify (record,
                      ll::bind (&fib::FaceMetric::IncreaseNack, ll::_1));
          //NS_LOG_UNCOND("OnNack node="<<inFace->GetNode()->GetId()<<" fraction="<<record->GetFraction());             	      
      }
      
    
  }
  
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
