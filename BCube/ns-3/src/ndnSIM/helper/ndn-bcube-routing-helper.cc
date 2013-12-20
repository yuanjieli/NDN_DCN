/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 UCLA
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
 * Author:  Yuanjie Li <yuanjie.li@cs.ucla.edu>
 */

#include "ndn-bcube-routing-helper.h"

//#include "ns3/ndn-l3-protocol.h"
#include "../model/ndn-bcube-l3-protocol.h"
#include "../model/ndn-net-device-face.h"
#include "../model/ndn-global-router.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-fib.h"

#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/channel-list.h"
#include "ns3/object-factory.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/concept/assert.hpp>
// #include <boost/graph/graph_concepts.hpp>
// #include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "boost-graph-ndn-global-routing-helper.h"

#include <list>
#include <vector>

#include <math.h>

NS_LOG_COMPONENT_DEFINE ("ndn.BCubeRoutingHelper");

using namespace std;
using namespace boost;

namespace ns3 {
namespace ndn {

typedef std::vector<Ptr<Node> > TreeNode_t;
typedef TreeNode_t::iterator	TreeNodeIterator;
typedef std::vector<std::pair<Ptr<Node>, Ptr<Node> > > TreeLink_t;
typedef TreeLink_t::iterator TreeLinkIterator;

void
BCubeRoutingHelper::Install (Ptr<Node> node)
{
  NS_LOG_LOGIC ("Node: " << node->GetId ());

  Ptr<BCubeL3Protocol> ndn = node->GetObject<BCubeL3Protocol> ();
  NS_ASSERT_MSG (ndn != 0, "Cannot install BCubeRoutingHelper before Ndn is installed on a node");

  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter> ();
  if (gr != 0)
    {
      NS_LOG_DEBUG ("GlobalRouter is already installed: " << gr);
      return; // already installed
    }

  gr = CreateObject<GlobalRouter> ();
  node->AggregateObject (gr);

	//uploadlinks and download links share the device
	//Here we only need to consider upload lnk, so faceId += 2
  for (uint32_t faceId = 0; faceId < ndn->GetNFaces (); faceId += 2)	
    {
      Ptr<NetDeviceFace> face = DynamicCast<NetDeviceFace> (ndn->GetUploadFace (faceId));
      if (face == 0)
			{
			  NS_LOG_DEBUG ("Skipping non-netdevice face");
			  continue;
			}

      Ptr<NetDevice> nd = face->GetNetDevice ();
      if (nd == 0)
			{
			  NS_LOG_DEBUG ("Not a NetDevice associated with NetDeviceFace");
			  continue;
			}

      Ptr<Channel> ch = nd->GetChannel ();

      if (ch == 0)
			{
			  NS_LOG_DEBUG ("Channel is not associated with NetDevice");
			  continue;
			}

      //if (ch->GetNDevices () == 2) // e.g., point-to-point channel
      NS_ASSERT(ch->GetNDevices () == 2);	//BCube only uses point-to-point channel
			
	  for (uint32_t deviceId = 0; deviceId < ch->GetNDevices (); deviceId ++)
	  {
		 Ptr<NetDevice> otherSide = ch->GetDevice (deviceId);
		 if (nd == otherSide) continue;
		
		 Ptr<Node> otherNode = otherSide->GetNode ();
		 NS_ASSERT (otherNode != 0);
		
		 Ptr<GlobalRouter> otherGr = otherNode->GetObject<GlobalRouter> ();
		 if (otherGr == 0)
		 {
			Install (otherNode);
		 }
		 otherGr = otherNode->GetObject<GlobalRouter> ();
		 NS_ASSERT (otherGr != 0);
		 gr->AddIncidency (face, otherGr);
	   }
			     
    }
}

void
BCubeRoutingHelper::Install (Ptr<Channel> channel)
{
  NS_LOG_LOGIC ("Channel: " << channel->GetId ());

  Ptr<GlobalRouter> gr = channel->GetObject<GlobalRouter> ();
  if (gr != 0)
    return;

  gr = CreateObject<GlobalRouter> ();
  channel->AggregateObject (gr);

  for (uint32_t deviceId = 0; deviceId < channel->GetNDevices (); deviceId ++)
    {
      Ptr<NetDevice> dev = channel->GetDevice (deviceId);

      Ptr<Node> node = dev->GetNode ();
      NS_ASSERT (node != 0);

      Ptr<GlobalRouter> grOther = node->GetObject<GlobalRouter> ();
      if (grOther == 0)
			{
			  Install (node);
			}
      grOther = node->GetObject<GlobalRouter> ();
      NS_ASSERT (grOther != 0);

      gr->AddIncidency (0, grOther);
    }
}

void
BCubeRoutingHelper::Install (const NodeContainer &nodes)
{
  for (NodeContainer::Iterator node = nodes.Begin ();
       node != nodes.End ();
       node ++)
    {
      Install (*node);
    }
}

void
BCubeRoutingHelper::InstallAll ()
{
  Install (NodeContainer::GetGlobal ());
}


void
BCubeRoutingHelper::AddOrigin (const std::string &prefix, Ptr<Node> node)
{
  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter> ();
  NS_ASSERT_MSG (gr != 0,
		 "GlobalRouter is not installed on the node");

  Ptr<Name> name = Create<Name> (boost::lexical_cast<Name> (prefix));
  gr->AddLocalPrefix (name);
}

void
BCubeRoutingHelper::AddOrigins (const std::string &prefix, const NodeContainer &nodes)
{
  for (NodeContainer::Iterator node = nodes.Begin ();
       node != nodes.End ();
       node++)
    {
      AddOrigin (prefix, *node);
    }
}

void
BCubeRoutingHelper::AddOrigin (const std::string &prefix, const std::string &nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  NS_ASSERT_MSG (node != 0, nodeName << "is not a Node");

  AddOrigin (prefix, node);
}

void
BCubeRoutingHelper::AddOriginsForAll ()
{
  for (NodeList::Iterator node = NodeList::Begin (); node != NodeList::End (); node ++)
    {
      Ptr<GlobalRouter> gr = (*node)->GetObject<GlobalRouter> ();
      string name = Names::FindName (*node);

      if (gr != 0 && !name.empty ())
        {
          AddOrigin ("/"+name, *node);
        }
    }
}

void
BCubeRoutingHelper::CalculateRoutes ()
{
  /**
   * Implementation of route calculation is heavily based on Boost Graph Library
   * See http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/table_of_contents.html for more details
   */

  BOOST_CONCEPT_ASSERT(( VertexListGraphConcept< NdnGlobalRouterGraph > ));
  BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept< NdnGlobalRouterGraph > ));

  NdnGlobalRouterGraph graph;
  typedef graph_traits < NdnGlobalRouterGraph >::vertex_descriptor vertex_descriptor;

  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by the graph, which
  // is not obviously how implement in an efficient manner
  for (NodeList::Iterator node = NodeList::Begin (); node != NodeList::End (); node++)
    {
	      Ptr<GlobalRouter> source = (*node)->GetObject<GlobalRouter> ();
	      if (source == 0)
				{
				  NS_LOG_DEBUG ("Node " << (*node)->GetId () << " does not export GlobalRouter interface");
				  continue;
				}
	
	      DistancesMap    distances;
	
	      dijkstra_shortest_paths (graph, source,
				       // predecessor_map (boost::ref(predecessors))
				       // .
				       distance_map (boost::ref(distances))
				       .
				       distance_inf (WeightInf)
				       .
				       distance_zero (WeightZero)
				       .
				       distance_compare (boost::WeightCompare ())
				       .
				       distance_combine (boost::WeightCombine ())
				       );
	
	      // NS_LOG_DEBUG (predecessors.size () << ", " << distances.size ());
	
	      Ptr<Fib>  fib  = source->GetObject<Fib> ();
	      fib->InvalidateAll ();
	      NS_ASSERT (fib != 0);
	
	      NS_LOG_DEBUG ("Reachability from Node: " << source->GetObject<Node> ()->GetId ());
	      for (DistancesMap::iterator i = distances.begin ();
		   i != distances.end ();
		   i++)
			{
			  if (i->first == source)
			    continue;
			  else
			    {
			      // cout << "  Node " << i->first->GetObject<Node> ()->GetId ();
			      if (i->second.get<0> () == 0)
				{
				  // cout << " is unreachable" << endl;
				}
			      else
				{
		                  BOOST_FOREACH (const Ptr<const Name> &prefix, i->first->GetLocalPrefixes ())
		                    {
		                      NS_LOG_DEBUG (" prefix " << *prefix << " reachable via face " << *i->second.get<0> ()
		                                    << " with distance " << i->second.get<1> ()
		                                    << " with delay " << i->second.get<2> ());
		
		                      Ptr<fib::Entry> entry = fib->Add (prefix, i->second.get<0> (), i->second.get<1> ());
		                      entry->SetRealDelayToProducer (i->second.get<0> (), Seconds (i->second.get<2> ()));
		
		                      Ptr<Limits> faceLimits = i->second.get<0> ()->GetObject<Limits> ();
		
		                      Ptr<Limits> fibLimits = entry->GetObject<Limits> ();
		                      if (fibLimits != 0)
		                        {
		                          // if it was created by the forwarding strategy via DidAddFibEntry event
		                          fibLimits->SetLimits (faceLimits->GetMaxRate (), 2 * i->second.get<2> () /*exact RTT*/);
		                          NS_LOG_DEBUG ("Set limit for prefix " << *prefix << " " << faceLimits->GetMaxRate () << " / " <<
		                                        2*i->second.get<2> () << "s (" << faceLimits->GetMaxRate () * 2 * i->second.get<2> () << ")");
		                        }
		                    }
				}
			    }
			}
    }
}

//Auxiliary function for extracting BCubeID from node's name
void
ExtractBCubeID(std::string &str, uint32_t *array)
{	
	for(size_t k=1; k != str.size(); k++)
	{
		array[k-1] = str[k]-'0';
	}
}
void
BCubeRoutingHelper::CalculateBCubeRoutes(uint32_t m_n, uint32_t m_k)
{
	//For simplification of simulation, we have some limits for n and k
  	NS_ASSERT(m_n>=1 && m_n<MAX_N);	
  	NS_ASSERT(m_k>=0 && m_k<MAX_K);
  	
	for(NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++)
	{
		/* Step 1: for each node, if it has local prefixes,
		 * calculate multiple disjoint Steiner trees to all nodes.
		 * The algorithm is from BCube paper (one-to-all communication).
		 * Even if this node has multiple local prefixes, we only need to calculate
		 * Steiner trees ONCE, and install FIBs based on these trees.
		 */
		Ptr<GlobalRouter> source = (*node)->GetObject<GlobalRouter> ();
	    if (source == 0)
		{
			NS_LOG_DEBUG ("Node " << (*node)->GetId () << " does not export GlobalRouter interface");
			continue;
		}
		//Guarantee that this node is really a server
		//Should ALWAYS be true because only switches don't install GlobalRouter
		std::string src_name = Names::FindName(*node);
		NS_ASSERT(src_name[0]=='S' && src_name.size() == m_n+1);
		
		//Extract source's BCubeID
		uint32_t src_addr[MAX_N];
		ExtractBCubeID(src_name, src_addr);
		
		if(source->GetLocalPrefixes().empty()) continue;	//no local prefixes
		
		//for(size_t level = 0; level <= m_k; level++)
		size_t level = 0;	//As first step, let's create one spanning tree only
		{
			//create root for this spanning tree
			std::string root_name = src_name;
			root_name[level] = '0' + (src_addr[level]+1)%m_n;
			Ptr<Node> root = Names::Find<Node>(root_name);
			NS_ASSERT(root != 0);
			TreeNode_t TreeNode;
			TreeNode.push_back(root); 
			//BuildSingSPT: Part I
			for(size_t i = 0; i <= m_k; i++)
			{
				size_t dim = (level+i)%(m_k+1);
				TreeLink_t TreeLink;
				for(TreeNodeIterator it = TreeNode.begin(); it != TreeNode.end(); it++)
				{
					Ptr<Node> B = *it, C = *it;
					std::string C_name = Names::FindName(C);
					for(size_t j = 0; j < m_n-1; j ++)
					{
						C_name[dim] = '0' + (C_name[dim]-'0'+1)%m_n;
						C = Names::Find<Node>(C_name);
						NS_ASSERT(C != 0);
						TreeLink.push_back(std::make_pair(B, C));
						B = C;
					}
				}
				
			}	
		}
				
	}
}

} // namespace ndn
} // namespace ns3
