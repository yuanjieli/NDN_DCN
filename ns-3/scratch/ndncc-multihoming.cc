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
// ndncc-multihoming.cc One consumer with two producers
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int 
main (int argc, char *argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("20"));
  Config::SetDefault ("ns3::ndn::fw::Nacks::EnableNACKs", BooleanValue (true));
  //Config::SetDefault ("ns3::ndn::Limits::LimitsDeltaRate::UpdateInterval", StringValue ("0.1"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::NackFeedback", StringValue ("2"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (4);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue ("2Mbps"));
  p2p.Install (nodes.Get (0), nodes.Get (1));
  p2p.SetDeviceAttribute("DataRate", StringValue ("1Mbps"));
  p2p.Install (nodes.Get (1), nodes.Get (2));
  p2p.Install (nodes.Get (1), nodes.Get (3));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC");
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.EnableLimits(true,Seconds(0.2),40,1100);
  ndnHelper.InstallAll ();
  
  //Add routes here
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.Install (nodes);
  ndnGlobalRoutingHelper.AddOrigins ("/prefix", nodes.Get (2));
  ndnGlobalRoutingHelper.AddOrigins ("/prefix", nodes.Get (3));
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes ();

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  ApplicationContainer consumers;
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/prefix");
  consumerHelper.SetAttribute ("NackFeedback", StringValue ("1.1")); 
  consumerHelper.SetAttribute ("InitLimit", StringValue ("1.0")); 
  consumers = consumerHelper.Install (nodes.Get (0)); // first node
  consumers.Start (Seconds (1));	
  consumers.Stop (Seconds (400));
  
  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (nodes.Get (2)); // last node
  producerHelper.Install (nodes.Get (3)); // last node
  
  



  Simulator::Stop (Seconds (400.0));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
