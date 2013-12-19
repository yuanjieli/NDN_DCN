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

#ifndef NDN_BCUBE_ROUTING_HELPER_H
#define NDN_BCUBE_ROUTING_HELPER_H

#include "ns3/ptr.h"
#include <string>

#define MAX_N 10
#define MAX_K 10

namespace ns3 {

class Node;
class NodeContainer;
class Channel;

namespace ndn {

/**
 * @ingroup ndn
 * @brief The routing is based on one-to-many disjoint Steiner Tree algorithm (ref. BCube paper)
 */
class BCubeRoutingHelper
{
public:
  BCubeRoutingHelper(uint32_t nn, uint32_t kk):
  m_n (nn), m_k (kk) 
  {
  	//For simplification of simulation, we have some limits for n and k
  	NS_ASSERT(nn>=1 && nn<MAX_N);	
  	NS_ASSERT(kk>=0 && kk<MAX_K);
  }
  /**
   * @brief Install GlobalRouter interface on a node
   *
   * Note that GlobalRouter will also be installed on all connected nodes and channels
   *
   * @param node Node to install GlobalRouter interface
   */
  void
  Install (Ptr<Node> node);


  /**
   * @brief Install GlobalRouter interface on nodes
   *
   * Note that GlobalRouter will also be installed on all connected nodes and channels
   *
   * @param nodes NodeContainer to install GlobalRouter interface
   */
  void
  Install (const NodeContainer &nodes);

  /**
   * @brief Install GlobalRouter interface on all nodes
   */
  void
  InstallAll ();

  /**
   * @brief Add `prefix' as origin on `node'
   * @param prefix Prefix that is originated by node, e.g., node is a producer for this prefix
   * @param node   Pointer to a node
   */
  void
  AddOrigin (const std::string &prefix, Ptr<Node> node);

  /**
   * @brief Add `prefix' as origin on all `nodes'
   * @param prefix Prefix that is originated by nodes
   * @param nodes NodeContainer
   */
  void
  AddOrigins (const std::string &prefix, const NodeContainer &nodes);

  /**
   * @brief Add `prefix' as origin on node `nodeName'
   * @param prefix     Prefix that is originated by node, e.g., node is a producer for this prefix
   * @param nodeName   Name of the node that is associated with Ptr<Node> using ns3::Names
   */
  void
  AddOrigin (const std::string &prefix, const std::string &nodeName);

  /**
   * @brief Add origin to each node based on the node's name (using Names class)
   */
  void
  AddOriginsForAll ();

  /**
   * @brief Calculate for every node shortest path trees and install routes to all prefix origins
   */
  static void
  CalculateRoutes (); 
  
  void
  CalculateBCubeRoutes ();
  
  

private:
  BCubeRoutingHelper();	//parameters for BCube are mandatory
  void
  Install (Ptr<Channel> channel);
  
  //BCube parameters. These parameters SHOULD be consistent with the SIGCOMM paper
  uint32_t m_n;	//#ports for each switch
  uint32_t m_k;	//#levels. 
  
};

} // namespace ndn
} // namespace ns3

#endif // NDN_BCUBE_ROUTING_HELPER_H
