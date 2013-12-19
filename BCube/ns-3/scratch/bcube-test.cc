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

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (4);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue ("1Mbps"));
  p2p.Install (nodes.Get (0), nodes.Get (1));
  p2p.Install (nodes.Get (0), nodes.Get (2));
  p2p.Install (nodes.Get (0), nodes.Get (3));
  
  
  ndn::SwitchStackHelper switchHelper;
  switchHelper.Install (nodes.Get (0));	
  
  ndn::BCubeStackHelper ndnHelper;  
  ndnHelper.Install (nodes.Get (1));
  ndnHelper.Install (nodes.Get (2));
  ndnHelper.Install (nodes.Get (3));
   	
  
  // Installing applications

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (nodes.Get (3));   
  
  //Add routes here
  ndn::BCubeStackHelper::AddRoute (nodes.Get(1),"/prefix",0,01);
  ndn::BCubeStackHelper::AddRoute (nodes.Get(2),"/prefix",0,12);
  
  
  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  ApplicationContainer consumers;
  consumerHelper.SetPrefix ("/prefix");
  consumerHelper.SetAttribute ("Frequency", StringValue("0.5"));
  consumers = consumerHelper.Install (nodes.Get (1)); 
  consumers.Start (Seconds (0));	
  consumers.Stop (Seconds (simulation_time));
   
  Simulator::Run ();
  Simulator::Destroy ();
  	
  return 0;
}
