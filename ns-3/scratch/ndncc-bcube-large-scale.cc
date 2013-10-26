/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
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
// ndncc-multipath-fairness.cc Two consumers. One has two producers, the other has just one
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/log.h"

#include <cstdio>
#include <set>
#include <ctime>
#include <string>
#include <fstream>
#include <sstream>
#include <string>

using namespace ns3;

/*void
ShowFinishTime()
{
		if(Simulator::IsFinished())
		{
			NS_LOG_UNCOND("Simulation finishes at t="<<Simulator::Now().GetSeconds());
			return;
		}
}*/

long packet_loss = 0;
long packet_sent = 0;
static void
DropPacket (std::string str, Ptr<const Packet> packet)
{
		packet_loss ++;
}

static void
EnqueuePacket (std::string str, Ptr<const Packet> packet)
{
		packet_sent ++;
}

int 
main (int argc, char *argv[])
{
	int simulation_time = 400;
	//int no_flow = 2;	//number of DATA flows, at most 8 (we have 16 servers)
	srand(time(0));
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("50"));
  Config::SetDefault ("ns3::ndn::fw::Nacks::EnableNACKs", BooleanValue (true));
  Config::SetDefault ("ns3::ndn::Limits::LimitsDeltaRate::UpdateInterval", StringValue ("1.0")); 
  Config::SetDefault ("ns3::ndn::ConsumerOm::DataFeedback", StringValue ("1000"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::NackFeedback", StringValue ("1.5"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1.0"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::InitLimit", StringValue ("10.0"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
	//Read topology from BCube
	AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-3.txt");
  topologyReader.Read ();
  
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC");
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.EnableLimits(true,Seconds(0.2),40,1100);
  ndnHelper.InstallAll ();
  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  
  std::ifstream fin;
  fin.open("src/ndnSIM/examples/topologies/bcube-4-3-flow-immutable");
  std::string consumer, producer;
  long no_packets;
  
  while(fin>>consumer>>producer>>no_packets)
  {
  	//Add producer here
  	std::stringstream prefix;
  	prefix<<"/prefix"<<producer.substr(1,producer.size()-1);
  	Ptr<Node> server = Names::Find<Node> (producer.c_str());
  	ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  	producerHelper.SetPrefix (prefix.str().c_str());
		producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
		producerHelper.Install (server);  
		
		//Add consumer here	
		ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  	ApplicationContainer consumers;
  	consumerHelper.SetPrefix (prefix.str().c_str());
  	consumerHelper.SetAttribute ("MaxSeq", IntegerValue(no_packets));
		consumers = consumerHelper.Install (Names::Find<Node> (consumer)); 
		consumers.Start (Seconds (0));	
    consumers.Stop (Seconds (simulation_time));
			
		//Add routing table here
		ndnGlobalRoutingHelper.AddOrigins (prefix.str(), server);
		
		//NS_LOG_UNCOND(consumer<<" "<<producer<<" "<<prefix.str());
  }
  
  fin.close();
  
  //Calculate global routing
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes ();
  //ndnGlobalRoutingHelper.CalculateRoutes ();
  
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback (&DropPacket));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/TxQueue/Enqueue", MakeCallback (&EnqueuePacket));
  
  Simulator::Stop (Seconds (simulation_time));
  	
  //Simulator::ScheduleNow(ShowFinishTime);

  Simulator::Run ();
  Simulator::Destroy ();
  	
  NS_LOG_UNCOND("Total_packet="<<packet_sent<<" packet_loss="<<packet_loss<<" loss_ratio="<<packet_loss*100/packet_sent<<"%");
  	
  return 0;
}
