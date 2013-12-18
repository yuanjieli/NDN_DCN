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

#ifndef NDN_SWITCH_STACK_HELPER_H
#define NDN_SWITCH_STACK_HELPER_H

#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ns3/nstime.h"

namespace ns3 {

class Node;

namespace ndn {

class FaceContainer;
class Face;
class NetDeviceFace;

/* WARNING: FOR EACH NETDEVICE, WE SHOULD CREATE TWO FACES ASSOCIATED WITH IT: DOWNLOAD/UPLOAD LINK
 */
typedef std::pair<Ptr<NetDeviceFace>, Ptr<NetDeviceFace> > FacePair;
/**
 * \ingroup ndn
 * \defgroup ndn-helpers Helpers for BCube switch
 */
/**
 * \ingroup ndn-helpers
 * \brief Adding BCube switch functionality to existing Nodes.
 *
 */
class SwitchStackHelper
{
public:
	
  /**
   * \brief Create a new NdnStackHelper with a default NDN_FLOODING forwarding stategy
   */
  SwitchStackHelper();

  /**
   * \brief Destroy the NdnStackHelper
   */
  virtual ~SwitchStackHelper ();

  /**
   * \brief Install Ndn stack on the node
   *
   * This method will assert if called on a node that already has Ndn object
   * installed on it
   *
   * \param nodeName The name of the node on which to install the stack.
   *
   * \returns list of installed faces in the form of a smart pointer
   * to NdnFaceContainer object
   */
  Ptr<FaceContainer>
  Install (const std::string &nodeName) const;

  /**
   * \brief Install Ndn stack on the node
   *
   * This method will assert if called on a node that already has Ndn object
   * installed on it
   *
   * \param node The node on which to install the stack.
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  Install (Ptr<Node> node) const;

  /**
   * \brief Install Ndn stack on each node in the input container
   *
   * The program will assert if this method is called on a container with a node
   * that already has an ndn object aggregated to it.
   *
   * \param c NodeContainer that holds the set of nodes on which to install the
   * new stacks.
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  Install (const NodeContainer &c) const;

  /**
   * \brief Install Ndn stack on all nodes in the simulation
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  InstallAll () const;


private:

  FacePair
  PointToPointNetDevice (Ptr<Node> node, Ptr<L2Protocol> L2, Ptr<NetDevice> netDevice) const;
  

private:
  SwitchStackHelper (const SwitchStackHelper &);
  SwitchStackHelper &operator = (const SwitchStackHelper &o);

private:
  ObjectFactory m_ndnFactory;
  ObjectFactory m_strategyFactory;
  ObjectFactory m_contentStoreFactory;
  ObjectFactory m_pitFactory;
  ObjectFactory m_fibFactory;
  ObjectFactory m_fib2Factory;
  
  ObjectFactory m_L2Factory;	//L2Protocol module

  bool     m_limitsEnabled;
  Time     m_avgRtt;
  uint32_t m_avgContentObjectSize;
  uint32_t m_avgInterestSize;
  bool     m_needSetDefaultRoutes;

};

} // namespace ndn
} // namespace ns3

#endif /* NDN_SWITCH_STACK_HELPER_H */
