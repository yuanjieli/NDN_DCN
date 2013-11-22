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
// ndncc-cooperation.cc Proof of concept for our cooperation scheme
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/log.h"

#include <string>


using namespace ns3;

int 
main (int argc, char *argv[])
{
	int simulation_time = 100;
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("1ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("50"));
  Config::SetDefault ("ns3::ndn::fw::Nacks::EnableNACKs", BooleanValue (true));
  Config::SetDefault ("ns3::ndn::Limits::LimitsDeltaRate::UpdateInterval", StringValue ("1.0")); //This parameter is essential for fairness! We should analyze it.
  Config::SetDefault ("ns3::ndn::ConsumerOm::NackFeedback", StringValue ("1"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::DataFeedback", StringValue ("100"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1.0"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::InitLimit", StringValue ("10.0"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 1);
  topologyReader.SetFileName ("scratch/cooperation_map.txt");
  topologyReader.Read ();
  
  
  
  

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC");
  //ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Fifo", "MaxSize", "1000000");	//WARNING: HUGE IMPACT!
  ndnHelper.EnableLimits(true,Seconds(0.1),40,1100);
  ndnHelper.InstallAll ();
  
  // Installing applications

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix1");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (Names::Find<Node> ("S4")); 
  producerHelper.SetPrefix ("/prefix2");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (Names::Find<Node> ("S2")); 
  producerHelper.SetPrefix ("/prefix3");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (Names::Find<Node> ("S4")); 
  
  
  //Manually Add routes here, because we have non-shortest path
  //To be compatible with our code, set all paths with equal cost
  //S1
  /*ndn::StackHelper::AddRoute ("S1","/prefix1","S2",1);
  ndn::StackHelper::AddRoute ("S1","/prefix1","S3",1);	
  ndn::StackHelper::AddRoute ("S1","/prefix2","S2",1);	
  //S2
  ndn::StackHelper::AddRoute ("S2","/prefix1","S1",1);	
  ndn::StackHelper::AddRoute ("S2","/prefix1","S4",1);	
  //S3
  ndn::StackHelper::AddRoute ("S3","/prefix1","S1",1);	
  ndn::StackHelper::AddRoute ("S3","/prefix1","S4",1);	
  ndn::StackHelper::AddRoute ("S2","/prefix3","S4",1);*/
  	
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigins ("/prefix1", Names::Find<Node> ("S4"));
  ndnGlobalRoutingHelper.AddOrigins ("/prefix2", Names::Find<Node> ("S2"));
  ndnGlobalRoutingHelper.AddOrigins ("/prefix3", Names::Find<Node> ("S4"));
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes ();
  ndnGlobalRoutingHelper.CalculateFIB2 ();			
  	
  
  
  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  ApplicationContainer consumers;
  consumerHelper.SetPrefix ("/prefix1");
  consumers = consumerHelper.Install (Names::Find<Node> ("S1")); 
  consumers.Start (Seconds (0));	
  consumers.Stop (Seconds (simulation_time));
  
  consumerHelper.SetPrefix ("/prefix1");
  consumers = consumerHelper.Install (Names::Find<Node> ("S2")); 
  consumers.Start (Seconds (0));	
  consumers.Stop (Seconds (simulation_time));
  

  Simulator::Stop (Seconds (simulation_time));

  Simulator::Run ();
  Simulator::Destroy ();
  	
  return 0;
}
