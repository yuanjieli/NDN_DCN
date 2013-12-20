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
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
  //Read topology from BCube
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/bcube-4-2.txt");
  topologyReader.Read ();
  
  ndn::SwitchStackHelper switchHelper;
  switchHelper.InstallAll ();
  
  // Install NDN stack on all servers
  ndn::BCubeStackHelper ndnHelper;
  ndnHelper.InstallAll ();	//We will only install BCubeStackHelper to servers
  
  ndn::BCubeRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigin ("/prefix", Names::Find<Node>("S00"));
  ndnGlobalRoutingHelper.CalculateBCubeRoutes (4,2);
    	
  return 0;
}
