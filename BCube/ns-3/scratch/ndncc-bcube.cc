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
#include <cstdio>
#include <set>
#include <ctime>
#include <string>

using namespace ns3;

/*void 
ShowLimit(std::string context, Ptr<Node> node, uint32_t appID, Time consumer_time, double limit)
{
	std::cout<<"haha"<<std::endl;
	NS_LOG_UNCOND(node->GetId()<<" "<<appID<<" "<<limit);
}*/

int 
main (int argc, char *argv[])
{
	int simulation_time = 400;
	//int no_flow = 2;	//number of DATA flows, at most 8 (we have 16 servers)
	srand(time(0));
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::ndn::fw::Nacks::EnableNACKs", BooleanValue (true));
  Config::SetDefault ("ns3::ndn::Limits::LimitsDeltaRate::UpdateInterval", StringValue ("1.0")); 
  Config::SetDefault ("ns3::ndn::ConsumerOm::NackFeedback", StringValue ("1.5"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::LimitInterval", StringValue ("1.0"));
  Config::SetDefault ("ns3::ndn::ConsumerOm::InitLimit", StringValue ("10.0"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
	//Read topology from BCube
	AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-1.txt");
  topologyReader.Read ();
  
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC");
  ndnHelper.SetForwardingStrategy("ns3::ndn::fw::BestCC::PerOutFaceDeltaLimits");
  ndnHelper.EnableLimits(true,Seconds(0.2),40,1100);
  ndnHelper.InstallAll ();
  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  
  //Add producer here
  for(int i=0;i!=4;i++)
  	for(int j=0;j!=4;j++)
  	{
  		char server_name[50];
  		char prefix[50];
  		sprintf(server_name,"S%d%d",i,j);
  		sprintf(prefix,"/prefix%d%d",i,j);
  		Ptr<Node> server = Names::Find<Node> (server_name);
  		ndnGlobalRoutingHelper.AddOrigins (prefix, server);
  		ndn::AppHelper producerHelper ("ns3::ndn::Producer");
		  // Producer will reply to all requests starting with /prefix
		  producerHelper.SetPrefix (prefix);
		  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
		  producerHelper.Install (server);  		
  	}
  
  //Calculate global routing
  ndnGlobalRoutingHelper.CalculateAllPossibleRoutes ();
  //ndnGlobalRoutingHelper.CalculateRoutes ();

  // Installing applications

  // Consumer
  /*ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  ApplicationContainer consumers;
  std::set<int> setExclude;
  for(int k=0;k!=no_flow;k++)
  {
  	int v1, v2;
  	while(true)
  	{
  		v1 = rand()%16;
  		if(setExclude.find(v1)==setExclude.end())
  		{
  			setExclude.insert(v1);
  			break;
  		}
  	}
  	while(true)
  	{
  		v2 = rand()%16;
  		if( v2!=v1 && setExclude.find(v2)==setExclude.end())
  		{
  			setExclude.insert(v2);
  			break;
  		}
  	}
  	
  	char server[50], prefix[50];
  	sprintf(server,"S%d%d",v1/4,v1%4);
  	sprintf(prefix,"/prefix%d%d",v2/4,v2%4);
  	
  	consumerHelper.SetPrefix (prefix);
  	consumers = consumerHelper.Install (Names::Find<Node> (server)); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
   
			
    //NS_LOG_UNCOND(server<<" wants "<<prefix);  	
  }*/
  
  //all-to-all communication
  /*ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  ApplicationContainer consumers;
  for(int i=0;i!=4;i++)
  	for(int j=0;j!=4;j++)
  	{
  		char server[50];
  		sprintf(server,"S%d%d",i,j);
  		for(int i1=0;i1!=4;i1++)
  			for(int j1=0;j1!=4;j1++)
  			{
  				if(i1==i && j1==j)	//Do not send data to itself
  					continue;					
  				char prefix[50];
  				sprintf(prefix,"/prefix%d%d",i1,j1);
  				consumerHelper.SetPrefix (prefix);
			  	consumers = consumerHelper.Install (Names::Find<Node> (server)); 
			  	consumers.Start (Seconds (i*4+j));	
			    consumers.Stop (Seconds (simulation_time));
			     					
  			}
  	}*/
  	
  	//manual configuration
  	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerOm");
  	ApplicationContainer consumers;
  	consumerHelper.SetPrefix ("/prefix11");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S30")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix31");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S32")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    /*consumerHelper.SetPrefix ("/prefix13");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S30")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix31");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S02")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix00");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S22")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix12");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S32")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix11");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S10")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));
    consumerHelper.SetPrefix ("/prefix21");
  	consumers = consumerHelper.Install (Names::Find<Node> ("S33")); 
  	consumers.Start (Seconds (1));	
    consumers.Stop (Seconds (simulation_time));*/

 
  Simulator::Stop (Seconds (simulation_time));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
