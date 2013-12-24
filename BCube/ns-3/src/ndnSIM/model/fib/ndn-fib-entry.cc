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
#include "ns3/names.h"

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

/* The metric is encoded as follows:
 *  |level_i|nexthop_i|level_(i-1)|nexthop_(i-1)|...|#paths|
 * 	We have to encode this way, because one face may be used for multiple one-way routing
 */
void
Entry::AddOrUpdateRoutingMetric (Ptr<Face> face, int32_t metric)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (face != NULL, "Trying to Add or Update NULL face");

  FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
  if (record == m_faces.get<i_face> ().end ())
    {
      NS_LOG_UNCOND("metric="<<metric*10+1);
      m_faces.insert (FaceMetric (face, metric*10+1));	//first metric
    }
  else
  {
  	//FIXME: we may update metric to higher value
    // don't update metric to higher value
    /*if (record->GetRoutingCost () > metric || record->GetStatus () == FaceMetric::NDN_FIB_RED)
      {
        m_faces.modify (record,
                        ll::bind (&FaceMetric::SetRoutingCost, ll::_1, metric));

        m_faces.modify (record,
                        ll::bind (&FaceMetric::SetStatus, ll::_1, FaceMetric::NDN_FIB_YELLOW));
      }*/
    //For BCube(8,3), we need at most (3+1)*2+1=9 digits, so int32_t is just enough 
    NS_LOG_UNCOND("Aha!");
	m_faces.modify(record, 
				   ll::bind (&FaceMetric::SetRoutingCost, ll::_1,
				   	record->GetRoutingCost ()%10+1	//#faces
				   +(record->GetRoutingCost () - record->GetRoutingCost ()%10)*100	//previous routes (two-digit metric)	
				   +metric*10	//new metrics
				   ));
	m_faces.modify (record,
                        ll::bind (&FaceMetric::SetStatus, ll::_1, FaceMetric::NDN_FIB_YELLOW));
  }

  // reordering random access index same way as by metric index
  m_faces.get<i_nth> ().rearrange (m_faces.get<i_metric> ().begin ());
}

int32_t
Entry::GetRoutingMetric(Ptr<Face> face)
{
	FaceMetricByFace::type::iterator record = m_faces.get<i_face> ().find (face);
	if(record == m_faces.get<i_face> ().end())
		return -1;
	else
		return record->GetRoutingCost();
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
Entry::ShowRate()
{
	//FIXME: To show throughput, change the condition here!
	/*if(m_faces.begin()->GetFace()->GetNode()->GetId() <= 2
	&&(*m_prefix=="/prefix1" || *m_prefix=="/prefix2"))
		NS_LOG_UNCOND(m_faces.begin()->GetFace()->GetNode()->GetId()<<" "
							    <<Simulator::Now().GetSeconds()<<" "
							    <<m_data/109.5);*/
	m_data = 0;
	
	Simulator::Schedule(Seconds(SHOW_RATE_INTERVAL), &Entry::ShowRate, this);
	
}
void
Entry::ResetCount()
{
	//reset overall data rate
	
	//reset each face's count
	for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {
    	
    		/*if(Names::FindName(face->GetFace()->GetNode())=="S10")
	    	NS_LOG_UNCOND(Names::FindName(face->GetFace()->GetNode())
	    				<<" prefix="<<*m_prefix
	    				<<" faceID="<<face->GetFace()->GetId()
	    				<<" fraction="<<face->GetFraction()
	    				<<" interest="<<face->GetInterest()
	    				<<" NACK="<<face->GetNack()
	    				<<" Data_in="<<face->GetDataIn()
	    			  //<<" Data_CE="<<face->GetDataCE()
	    				);*/
	    								
      m_faces.modify (face,
                      ll::bind (&FaceMetric::ResetCounter, ll::_1));
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
    	q_mean += face->GetNackOld(); 
	    facecount ++;   								
    }
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    {  
    		double tmp = face->GetFraction()*q_mean/100.0-face->GetNackOld();
    		q_var += tmp*tmp;
    		if(tmp>0)
    		{
    			double tmp2 = (w_upper_bound-face->GetFraction())/tmp;
    			if(K_bound>tmp2)
    				K_bound = tmp2;
    				
    		}
    		else if(tmp<0)
    		{
    			double tmp2 = (w_lower_bound-face->GetFraction())/tmp;
    			if(K_bound>tmp2)
    				K_bound = tmp2;
    				
    		}							
    }
  q_var = sqrt(q_var)/facecount;
  K = K_bound*tanh(q_var/(1+q_mean)/5);  
  //K = K_bound*tanh(q_var/(1+q_mean)/2);  
  
  for (FaceMetricByFace::type::iterator face = m_faces.begin ();
       face != m_faces.end ();
       face++)
    { 
    	
	      if(m_inited)
	      {
		      double fraction = face->GetFraction()
		      								+ K * (face->GetFraction()*q_mean/100.0-face->GetNackOld());
		      m_faces.modify (face,
		                      ll::bind (&FaceMetric::SetFraction, ll::_1,fraction));
		    }
		    else
		    {
	    		m_faces.modify (face,
	                      ll::bind (&FaceMetric::SetFraction, ll::_1,100.0/facecount));
		    }  	
    									
    }  
  m_inited = true; 
  
    
 
  Simulator::Schedule(Seconds(UPDATE_INTERVAL), &Entry::ResetCount, this);
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
