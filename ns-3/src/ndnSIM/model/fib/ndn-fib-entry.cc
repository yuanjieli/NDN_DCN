/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ndn-fib-entry.h"

#include "ns3/ndn-name.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/node.h"

#include <cmath>

#define NDN_RTO_ALPHA 0.125
#define NDN_RTO_BETA 0.25
#define NDN_RTO_K 4

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;
	

NS_LOG_COMPONENT_DEFINE ("ndn.fib.Entry");

#define MAX_BOUND 10000000

namespace ns3 {
namespace ndn {
namespace fib {

//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

struct FaceMetricByFace
{
  typedef FaceMetricContainer::type::index<i_face>::type
  type;
};


void
FaceMetric::UpdateRtt (const Time &rttSample)
{
  // const Time & this->m_rttSample

  //update srtt and rttvar (RFC 2988)
  if (m_sRtt.IsZero ())
    {
      //first RTT measurement
      NS_ASSERT_MSG (m_rttVar.IsZero (), "SRTT is zero, but variation is not");

      m_sRtt = rttSample;
      m_rttVar = Time (m_sRtt / 2.0);
    }
  else
    {
      m_rttVar = Time ((1 - NDN_RTO_BETA) * m_rttVar + NDN_RTO_BETA * Abs(m_sRtt - rttSample));
      m_sRtt = Time ((1 - NDN_RTO_ALPHA) * m_sRtt + NDN_RTO_ALPHA * rttSample);
    }
}

/////////////////////////////////////////////////////////////////////

void
Entry::UpdateFaceRtt (Ptr<Face> face, const Time &sample)
{
  FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  NS_ASSERT_MSG (record != m_faces.get<i_face> ().end (),
                 "Update status can be performed only on existing faces of CcxnFibEntry");

  m_faces.modify (record,
                  ll::bind (&FaceMetric::UpdateRtt, ll::_1, sample));

  // reordering random access index same way as by metric index
  m_faces.get<i_nth> ().rearrange (m_faces.get<i_metric> ().begin ());
}

void
Entry::UpdateStatus (Ptr<Face> face, FaceMetric::Status status)
{
  NS_LOG_FUNCTION (this << boost::cref(*face) << status);

  FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  NS_ASSERT_MSG (record != m_faces.get<i_face> ().end (),
                 "Update status can be performed only on existing faces of CcxnFibEntry");

  m_faces.modify (record,
                  ll::bind (&FaceMetric::SetStatus, ll::_1, status));

  // reordering random access index same way as by metric index
  m_faces.get<i_nth> ().rearrange (m_faces.get<i_metric> ().begin ());
}

void
Entry::AddOrUpdateRoutingMetric (Ptr<Face> face, int32_t metric)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (face != NULL, "Trying to Add or Update NULL face");

  FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  if (record == m_faces.get<i_face> ().end ())
    {
      m_faces.insert (FaceMetric (face, metric));
    }
  else
  {
    // don't update metric to higher value
    if (record->GetRoutingCost () > metric || record->GetStatus () == FaceMetric::NDN_FIB_RED)
      {
        m_faces.modify (record,
                        ll::bind (&FaceMetric::SetRoutingCost, ll::_1, metric));

        m_faces.modify (record,
                        ll::bind (&FaceMetric::SetStatus, ll::_1, FaceMetric::NDN_FIB_YELLOW));
      }
  }

  // reordering random access index same way as by metric index
  m_faces.get<i_nth> ().rearrange (m_faces.get<i_metric> ().begin ());
}

void
Entry::SetRealDelayToProducer (Ptr<Face> face, Time delay)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (face != NULL, "Trying to Update NULL face");

  FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  if (record != m_faces.get<i_face> ().end ())
    {
      m_faces.modify (record,
                      ll::bind (&FaceMetric::SetRealDelay, ll::_1, delay));
    }
}


void
Entry::Invalidate ()
{
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {
      m_faces.modify (face,
                      ll::bind (&FaceMetric::SetRoutingCost, ll::_1, std::numeric_limits<uint16_t>::max ()));

      m_faces.modify (face,
                      ll::bind (&FaceMetric::SetStatus, ll::_1, FaceMetric::NDN_FIB_RED));
    }
}

void
Entry::ResetCount()
{
	//reset overall data rate
	
	//FIXME: To show throughput, change the condition here!
	if(m_faces.begin()->GetFace()->GetNode()->GetId() <= 1
	&&(*m_prefix=="/prefix1" || *m_prefix=="/prefix2"))
		NS_LOG_UNCOND(m_faces.begin()->GetFace()->GetNode()->GetId()<<" "
							    <<Simulator::Now().GetSeconds()<<" "
							    <<m_data/109.5);
	m_data = 0;
	
	
	//reset each face's count
	for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {
    	
    	if(face->GetFace()->GetNode()->GetId()==0
    	|| face->GetFace()->GetNode()->GetId()==1)
	    	NS_LOG_UNCOND(
	    								"nodeID="<<face->GetFace()->GetNode()->GetId()
	    								<<" prefix="<<*m_prefix
	    								<<" faceID="<<face->GetFace()->GetId()
	    								<<" metric="<<face->GetFraction()
	    								<<" NACK="<<face->GetNack()
	    								//<<" Data_in="<<face->GetDataIn()
	    								//<<" Data_CE="<<face->GetDataCE()
	    								);
      m_faces.modify (face,
                      ll::bind (&FaceMetric::ResetCounter, ll::_1));
    }
    
  double minCost = MAX_BOUND;
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
  {
  	if(face->GetRoutingCost()<minCost)
  		minCost = face->GetRoutingCost();
  }
  double K_bound = MAX_BOUND;
  double w_lower_bound = 5;	//lower bound for probing traffic
  double w_upper_bound = 100;	//upper bound for w
  double K = 0;
  double q_var = 0;
  double q_mean = 0;
  uint32_t facecount = 0;
  
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {  
    	if(face->GetRoutingCost()==minCost)
    	{ 
    		q_mean += face->GetSharingMetric(); 
	    	facecount ++;
	    }    								
    }
  
  q_mean /= facecount;
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {  
    	if(face->GetRoutingCost()==minCost)
    	{
    		double tmp = face->GetSharingMetric()-q_mean;
    		q_var += tmp*tmp;
    		if(tmp>0)
    		{
    			//q_var += tmp;
    			double tmp2 = (w_upper_bound-face->GetFraction())/tmp;
    			if(K_bound>tmp2)
    				K_bound = tmp2;
    				
    		}
    		else if(tmp<0)
    		{
    			//q_var += -tmp;
    			double tmp2 = (w_lower_bound-face->GetFraction())/tmp;
    			if(K_bound>tmp2)
    				K_bound = tmp2;
    				
    		}
    	}							
    }
  //q_var /= facecount;
  q_var = sqrt(q_var/facecount);
  //K = K_bound*tanh(q_var/q_mean/5);  
  K = K_bound*tanh(q_var/q_mean); 
  
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    { 
    	if(face->GetRoutingCost()==minCost)
    	{
	      if(m_inited)
	      {
		      double fraction = face->GetFraction()
		      								+ K * (face->GetSharingMetric() - q_mean);
		      m_faces.modify (face,
		                      ll::bind (&FaceMetric::SetFraction, ll::_1,fraction));
		    }
		    else
		    {
    	              
	    		//NS_LOG_UNCOND("fraction="<<tmp*100/totalMetric);
	    		m_faces.modify (face,
	                      ll::bind (&FaceMetric::SetFraction, ll::_1,100/facecount));
		    }
    	}   	
    									
    }  
  m_inited = true; 
  
    
  //set fraction of traffic
  /*double minCost = 1000000;
  double K = 0.25;	//used for balancing congestion
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
  {
  	if(face->GetRoutingCost()<minCost)
  		minCost = face->GetRoutingCost();
  }
  double totalMetric = 0;
  double totalNack = 0;
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {  
    	if(face->GetRoutingCost()==minCost)
    	{  	
	    	totalMetric += face->GetSharingMetric();	
	    	totalNack += face->GetNack();
	    }    								
    }   
  
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    { 
    	if(face->GetRoutingCost()==minCost)
    	{
    		//proportional scheme
    		//double tmp = face->GetSharingMetric(); 
    	              
	    	//NS_LOG_UNCOND("fraction="<<tmp*100/totalMetric);
	    	//m_faces.modify (face,
	      //                ll::bind (&FaceMetric::SetFraction, ll::_1,100*tmp/totalMetric));
	                      
	      
	      //balance the congestion level
	      if(m_inited)
	      {
		      double fraction = face->GetFraction()
		      								+ K * (face->GetSharingMetric()*totalNack/totalMetric-face->GetNack());
		      m_faces.modify (face,
		                      ll::bind (&FaceMetric::SetFraction, ll::_1,fraction));
		    }
		    else
		    {
		    	double tmp = face->GetSharingMetric(); 
    	              
	    		//NS_LOG_UNCOND("fraction="<<tmp*100/totalMetric);
	    		m_faces.modify (face,
	                      ll::bind (&FaceMetric::SetFraction, ll::_1,100*tmp/totalMetric));
		    }
    	}   	
    									
    }  
  m_inited = true; */
  Simulator::Schedule(Seconds(1), &Entry::ResetCount, this);
}

const FaceMetric &
Entry::FindBestCandidate (uint32_t skip/* = 0*/) const
{
  if (m_faces.size () == 0) throw Entry::NoFaces ();
  skip = skip % m_faces.size();
  return m_faces.get<i_nth> () [skip];
}

std::ostream& operator<< (std::ostream& os, const Entry &entry)
{
  for (FaceMetricContainer::type::index<i_nth>::type::iterator metric =
         entry.m_faces.get<i_nth> ().begin ();
       metric != entry.m_faces.get<i_nth> ().end ();
       metric++)
    {
      if (metric != entry.m_faces.get<i_nth> ().begin ())
        os << ", ";

      os << *metric;
    }
  return os;
}

std::ostream& operator<< (std::ostream& os, const FaceMetric &metric)
{
  static const std::string statusString[] = {"","g","y","r"};

  os << *metric.m_face << "(" << metric.m_routingCost << ","<< statusString [metric.m_status] << "," << metric.m_face->GetMetric () << ")";
  return os;
}

} // namespace fib
} // namespace ndn
} // namespace ns3
