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

class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void 
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets || m_nPackets==0)
    {
      ScheduleTx ();
    }
}

void 
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

struct throughput_info{
	uint32_t totalbytes;
	Time start;
};

static std::map<std::string, throughput_info> mapThroughput;
static void
ShowThroughput (std::string str, Ptr<const Packet> packet, const Address &)
{
	std::map<std::string, throughput_info>::iterator it = mapThroughput.find(str);
	if(it == mapThroughput.end())
	{
		throughput_info new_info;
		new_info.start = Simulator::Now ();
		new_info.totalbytes = packet->GetSize ();
		mapThroughput[str] = new_info;
	}
	else
	{
		it->second.totalbytes += packet->GetSize ();
		Time interval = Simulator::Now () - it->second.start;
		if( interval >= Seconds(1))	//calculate throughput every second
		{
			//Get the node id
			unsigned pos = str.find("/ApplicationList");
			std::string id = str.substr(10,pos-10);
			
			double throughput = it->second.totalbytes*8.0/1000000.0/interval.GetSeconds();
			NS_LOG_UNCOND(id<<" "<<Simulator::Now().GetSeconds()<<" "<<throughput);
			
			//reset structure
			it->second.totalbytes = 0;
			it->second.start = Simulator::Now();		
		}
	}	
}

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
		//NS_LOG_UNCOND(packet_sent);
}

int 
main (int argc, char *argv[])
{
	
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
   Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));  
  	
	//Read topology from BCube
	AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("scratch/toy-example.txt");
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
  Names::Find<Node> ("S4")->AddApplication (bitTorrentTracker);	
  
  // 2) Load a torrent file via the tracker application
  Ptr<Torrent> sharedTorrent = bitTorrentTracker->AddTorrent ("input/bittorrent/torrent-data", "input/bittorrent/torrent-data/100MB-full.dat.torrent");
  
  // 3) Install BitTorrentClient applications on the desired number of nodes
  ApplicationContainer bitTorrentClients;
  Ptr<BitTorrentClient> client = Create<BitTorrentClient> ();
  client->SetTorrent (sharedTorrent);
  Names::Find<Node> ("S1")->AddApplication (client);
  bitTorrentClients.Add (client);	
  
  client = Create<BitTorrentClient> ();
  client->SetTorrent (sharedTorrent);
  Names::Find<Node> ("S2")->AddApplication (client);
  bitTorrentClients.Add (client);	
  
  /*client = Create<BitTorrentClient> ();
  client->SetTorrent (sharedTorrent);
  Names::Find<Node> ("S3")->AddApplication (client);
  bitTorrentClients.Add (client);*/	
  
  client = Create<BitTorrentClient> ();
  client->SetTorrent (sharedTorrent);
  Names::Find<Node> ("S4")->AddApplication (client);
  bitTorrentClients.Add (client);	
  DynamicCast<BitTorrentClient> (Names::Find<Node> ("S4")->GetApplication (1))->SetInitialBitfield ("full");
  
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



