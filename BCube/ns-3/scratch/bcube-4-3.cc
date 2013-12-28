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
  Config::SetDefault ("ns3::ndn::ConsumerOm::DataFeedback", StringValue ("20"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1.0"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::InitLimit", StringValue ("10.0"));
  
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
  //Read topology from BCube
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-3.txt");
  topologyReader.Read ();
  
  ndn::SwitchStackHelper switchHelper;
  switchHelper.InstallAll ();	//We will only install SwitchStackHelper to switches
  
  // Install NDN stack on all servers
  ndn::BCubeStackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Fifo", "MaxSize", "0");
  ndnHelper.EnableLimits(true,Seconds(0.1),40,10000);
  ndnHelper.InstallAll ();	//We will only install BCubeStackHelper to servers
  
  ndn::BCubeRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigin ("/prefix", Names::Find<Node>("S0000"));
  ndnGlobalRoutingHelper.CalculateBCubeRoutes (4,3);
  //ndnGlobalRoutingHelper.CalculateSharingRoutes (4,3);
  
  int simulation_time = 400;
   // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (Names::Find<Node>("S0000"));
  
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  //consumerHelper.SetAttribute("MaxSeq",IntegerValue(1000));	
  ApplicationContainer consumers;
  /*for(uint8_t i=0; i<4; i++)
  	for(uint8_t j=0; j<4; j++)
  		for(uint8_t k=0; k<4; k++)
  			for(uint8_t l=0; l<4; l++)
  			{
  				if(i==0 && j==0 && k==0 && l==0)continue;
  				//if(rand()%2==0)
  				if(rand()%4==0)
  				{
  					consumerHelper.SetPrefix ("/prefix");
	  				std::string str = "S";
	  				str += '0'+i;
	  				str += '0'+j;
	  				str += '0'+k;
	  				str += '0'+l;
	  				consumers = consumerHelper.Install (Names::Find<Node>(str)); 
	  				consumers.Start (Seconds (0));	
	  				consumers.Stop (Seconds (simulation_time));
  				}
  				
  			}*/
  consumerHelper.SetPrefix ("/prefix");	
  consumers = consumerHelper.Install (Names::Find<Node>("S3333")); 
	consumers.Start (Seconds (0));	
	consumers.Stop (Seconds (simulation_time));		
   
  Simulator::Stop (Seconds (simulation_time));

  Simulator::Run ();
  Simulator::Destroy ();
    	
  return 0;
}
