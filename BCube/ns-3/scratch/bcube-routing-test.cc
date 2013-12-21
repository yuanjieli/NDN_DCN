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

int 
main (int argc, char *argv[])
{	
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10us"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("50"));
  Config::SetDefault ("ns3::ndn::fw::Nacks::EnableNACKs", BooleanValue (true));
  Config::SetDefault ("ns3::ndn::Limits::LimitsDeltaRate::UpdateInterval", StringValue ("1.0")); //This parameter is essential for fairness! We should analyze it.
  Config::SetDefault ("ns3::ndn::ConsumerOm::NackFeedback", StringValue ("1"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::DataFeedback", StringValue ("200"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1.0"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::InitLimit", StringValue ("10.0"));
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
  //Read topology from BCube
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-1.txt");
  topologyReader.Read ();
  
  ndn::SwitchStackHelper switchHelper;
  switchHelper.InstallAll ();	//We will only install SwitchStackHelper to switches
  
  // Install NDN stack on all servers
  ndn::BCubeStackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Fifo", "MaxSize", "0");	//WARNING: HUGE IMPACT!
  ndnHelper.EnableLimits(true,Seconds(0.1),40,1100);
  ndnHelper.InstallAll ();	//We will only install BCubeStackHelper to servers
  
  ndn::BCubeRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigin ("/prefix", Names::Find<Node>("S00"));
  ndnGlobalRoutingHelper.CalculateBCubeRoutes (4,1);
  
  int simulation_time = 100;
   // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (Names::Find<Node>("S00"));
  
  //Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  ApplicationContainer consumers;
  consumerHelper.SetPrefix ("/prefix");
  consumers = consumerHelper.Install (Names::Find<Node>("S33")); 
  consumers.Start (Seconds (0));	
  consumers.Stop (Seconds (simulation_time));
  
  consumerHelper.SetPrefix ("/prefix");
  consumers = consumerHelper.Install (Names::Find<Node>("S30")); 
  consumers.Start (Seconds (0));	
  consumers.Stop (Seconds (simulation_time));
   
  Simulator::Stop (Seconds (simulation_time));

  Simulator::Run ();
  Simulator::Destroy ();
    	
  return 0;
}
