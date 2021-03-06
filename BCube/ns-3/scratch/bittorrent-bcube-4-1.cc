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
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include <cstdio>
#include <set>
#include <ctime>
#include <list>
#include <string>
#include <map>

#include "ns3/BitTorrentTracker.h"
#include "ns3/BitTorrentVideoClient.h"
#include "ns3/GlobalMetricsGatherer.h"


using namespace ns3;
using namespace bittorrent;

int 
main (int argc, char *argv[])
{
	
  CommandLine cmd;
  cmd.Parse (argc, argv);   
  	
	//Read topology from BCube
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-2-bitorrent.txt");
  NodeContainer nodes = topologyReader.Read ();
  
  InternetStackHelper internet;
  internet.InstallAll ();
  
  //Note: topologyReader only supports /24 netmask. So #nodes are limited!!!
  //Can change code later by referring topologyReader.cc
 	topologyReader.AssignIpv4Addresses("10.1.1.0"); 
 	
 	//Turn on global static routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
 	
  // 1) Install a BitTorrentTracker application (with default values) on one of the nodes
  Ptr<BitTorrentTracker> bitTorrentTracker = Create<BitTorrentTracker> ();
  Names::Find<Node> ("S000")->AddApplication (bitTorrentTracker);	
  
  // 2) Load a torrent file via the tracker application
  Ptr<Torrent> sharedTorrent = bitTorrentTracker->AddTorrent ("input/bittorrent/torrent-data", "input/bittorrent/torrent-data/10MB-full.dat.torrent");
  
  // 3) Install BitTorrentClient applications on the desired number of nodes
  ApplicationContainer bitTorrentClients;
  for(uint8_t i=0; i<4; i++)
  	for(uint8_t j=0; j<4; j++)
  		for(uint8_t k=0; k<4; k++)
  			{
  				std::string str = "S";
  				str += '0'+i;
  				str += '0'+j;
  				str += '0'+k;
  				Ptr<BitTorrentClient> client = Create<BitTorrentClient> ();
  				client->SetTorrent (sharedTorrent);	
  				Names::Find<Node> (str)->AddApplication (client);		
  				if(i==0 && j==0 && k==0)
  					DynamicCast<BitTorrentClient> (Names::Find<Node> (str)->GetApplication (1))->SetInitialBitfield ("full");
  			}
  
  // 4) Set up the BitTorrent metrics gatherer for output handling (here, we just log to the screen)
  GlobalMetricsGatherer* gatherer = GlobalMetricsGatherer::GetInstance ();
  gatherer->SetFileNamePrefix ("This will be ignored while logging to the screen", false);
  gatherer->RegisterWithApplications (bitTorrentClients);
  gatherer->SetStopFraction (1.0, 1.0); // Stops the simulation when all nodes have finished downloading
  
  
  
  // 5a) Start the simulation
  Simulator::Run ();

  Simulator::Destroy ();
  
  return 0;
}



