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
 * Author: Yuanjie Li <yuanjie.li@cs.ucla.edu>
 */

#include "ndn-limits-delta-rate.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/ndn-face.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE ("ndn.Limits.DeltaRate");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (LimitsDeltaRate);

TypeId
LimitsDeltaRate::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::Limits::LimitsDeltaRate")
    .SetGroupName ("Ndn")
    .SetParent <Limits> ()
    .AddConstructor <LimitsDeltaRate> ()
                   
    .AddAttribute ("UpdateInterval", "Interest rate varaition update Interval",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&LimitsDeltaRate::m_resetInterval),
                   MakeDoubleChecker<double> ())
    

    ;
  return tid;
}

LimitsDeltaRate::LimitsDeltaRate()
	: m_bucketMax (1)
  , m_bucket (0)
  , m_bucketOld (0)
  , m_resetInterval (1.0)
  , m_nack (0)
  //, m_oldnack (0)
{ 
	Simulator::Schedule (Seconds (m_resetInterval), &LimitsDeltaRate::UpdateBucket, this);	
}
	

void
LimitsDeltaRate::NotifyNewAggregate ()
{
  //do nothing
}

void
LimitsDeltaRate::SetLimits (double rate, double delay)
{
  super::SetLimits (rate, delay);

  // maximum allowed burst
  m_bucketMax = GetMaxRate () * m_resetInterval;	//scale the interest limit based on the reset interval
  
}


void
LimitsDeltaRate::UpdateCurrentLimit (double limit)
{
  NS_ASSERT_MSG (limit >= 0.0, "Limit should be greater or equal to zero");

  m_bucketMax  = limit * m_resetInterval; //scale the interest limit based on the reset interval
}

bool
LimitsDeltaRate::IsBelowLimit ()
{
  if (!IsEnabled ()) return true;

  return (m_bucketMax - m_bucket >= 1.0);
}

void
LimitsDeltaRate::BorrowLimit ()
{
  if (!IsEnabled ()) return;

  NS_ASSERT_MSG (m_bucketMax - m_bucket >= 1.0, "Should not be possible, unless we IsBelowLimit was not checked correctly");
  m_bucket += 1;
}

void
LimitsDeltaRate::ReturnLimit ()
{
  // do nothing
}

double 
LimitsDeltaRate::GetAvailableInterestIncrement () const
{
	double Delta = m_bucketMax - m_bucket; 
	Delta = Delta > 0 ? Delta : 0; 
	return Delta;	
	//return Delta/(m_nack+1);	
	//return 1/(m_nack+1);	//only care about whether remote links are congested as long as local links allow forwarding
}

void
LimitsDeltaRate::UpdateBucket ()
{
	Ptr<Face> iFace = GetObject<Face>();
	if(iFace==0)return;
	NS_LOG_INFO(iFace->GetNode()->GetId()<<" "<<iFace->GetId()<<" "<<m_bucket<<" "<<m_nack<<" "<<GetAvailableInterestIncrement());
	
	
		
	//Reset packet counter
	m_bucketOld = m_bucket;
	m_bucket = 0;	
	//m_oldnack = m_oldnack/8+m_nack*7/8;
	//m_nack = 0;
	
	
	//Schedule next round
	Simulator::Schedule (Seconds (m_resetInterval), &LimitsDeltaRate::UpdateBucket, this);	
}

} // namespace ndn
} // namespace ns3
