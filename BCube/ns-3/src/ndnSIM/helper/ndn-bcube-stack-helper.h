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

#ifndef NDN_BCUBE_STACK_HELPER_H
#define NDN_BCUBE_STACK_HELPER_H

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
//class L3Protocol;
class BCubeL3Protocol;

/**
 * \ingroup ndn
 * \defgroup ndn-helpers Helpers
 */
/**
 * \ingroup ndn-helpers
 * \brief Adding Ndn functionality to existing Nodes.
 *
 * Same as NDN StackHelper, except that for each NetDevice, we create two NetDeviceFace rather than one
 * 
 */
 
typedef std::pair< Ptr<NetDeviceFace>, Ptr<NetDeviceFace> > PairFace;
class BCubeStackHelper
{
public:
  /**
   * \brief Create a new NdnStackHelper with a default NDN_FLOODING forwarding stategy
   */
  BCubeStackHelper();

  /**
   * \brief Destroy the NdnStackHelper
   */
  virtual ~BCubeStackHelper ();

  /**
   * @brief Set parameters of NdnL3Protocol
   */
  void
  SetStackAttributes (const std::string &attr1 = "", const std::string &value1 = "",
                      const std::string &attr2 = "", const std::string &value2 = "",
                      const std::string &attr3 = "", const std::string &value3 = "",
                      const std::string &attr4 = "", const std::string &value4 = "");


  /**
   * @brief Set forwarding strategy class and its attributes
   * @param forwardingStrategyClass string containing name of the forwarding strategy class
   *
   * Valid options are "ns3::NdnFloodingStrategy" (default) and "ns3::NdnBestRouteStrategy"
   *
   * Other strategies can be implemented, inheriting ns3::NdnForwardingStrategy class
   */
  void
  SetForwardingStrategy (const std::string &forwardingStrategyClass,
                         const std::string &attr1 = "", const std::string &value1 = "",
                         const std::string &attr2 = "", const std::string &value2 = "",
                         const std::string &attr3 = "", const std::string &value3 = "",
                         const std::string &attr4 = "", const std::string &value4 = "");

  /**
   * @brief Set content store class and its attributes
   * @param contentStoreClass string, representing class of the content store
   */
  void
  SetContentStore (const std::string &contentStoreClass,
                   const std::string &attr1 = "", const std::string &value1 = "",
                   const std::string &attr2 = "", const std::string &value2 = "",
                   const std::string &attr3 = "", const std::string &value3 = "",
                   const std::string &attr4 = "", const std::string &value4 = "");

  /**
   * @brief Set PIT class and its attributes
   * @param pitClass string, representing class of PIT
   */
  void
  SetPit (const std::string &pitClass,
          const std::string &attr1 = "", const std::string &value1 = "",
          const std::string &attr2 = "", const std::string &value2 = "",
          const std::string &attr3 = "", const std::string &value3 = "",
          const std::string &attr4 = "", const std::string &value4 = "");

  /**
   * @brief Set FIB class and its attributes
   * @param pitClass string, representing class of FIB
   */
  void
  SetFib (const std::string &fibClass,
          const std::string &attr1 = "", const std::string &value1 = "",
          const std::string &attr2 = "", const std::string &value2 = "",
          const std::string &attr3 = "", const std::string &value3 = "",
          const std::string &attr4 = "", const std::string &value4 = "");
          	
          	
  /**
   * @brief Set FIB class and its attributes
   * @param pitClass string, representing class of FIB
   */
  void
  SetFib2 (const std::string &fib2Class,
          const std::string &attr1 = "", const std::string &value1 = "",
          const std::string &attr2 = "", const std::string &value2 = "",
          const std::string &attr3 = "", const std::string &value3 = "",
          const std::string &attr4 = "", const std::string &value4 = "");
  
  /**
   * @brief Enable Interest limits (disabled by default)
   *
   * @param enable           Enable or disable limits
   * @param avgRtt           Average RTT
   * @param avgContentObject Average size of contentObject packets (including all headers)
   * @param avgInterest      Average size of interest packets (including all headers)
   */
  void
  EnableLimits (bool enable = true, Time avgRtt=Seconds(0.1), uint32_t avgContentObject=1100, uint32_t avgInterest=40);

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


  static void
  AddRoute (Ptr<Node> node, const std::string &prefix, Ptr<Node> otherNode, int32_t metric);

  /**
   * @brief Add forwarding entry to FIB (work only with point-to-point links)
   *
   * \param nodeName Node name (refer to ns3::Names)
   * \param prefix Routing prefix
   * \param otherNode The other node name, to which interests (will be used to infer face id (refer to ns3::Names)
   * \param metric Routing metric
   */
  static void
  AddRoute (const std::string &nodeName, const std::string &prefix, const std::string &otherNodeName, int32_t metric);
  
  static void
  AddRoute (Ptr<Node> node, const std::string &prefix, uint32_t faceId, int32_t metric);
  		 
private:	
	static void
	AddRoute (Ptr<Node> node, const std::string &prefix, Ptr<Face> face, int32_t metric);	
			
  PairFace
  PointToPointNetDeviceCallBack(Ptr<Node> node, Ptr<BCubeL3Protocol> ndn, Ptr<NetDevice> netDevice) const;

private:
  BCubeStackHelper (const BCubeStackHelper &);
  BCubeStackHelper &operator = (const BCubeStackHelper &o);

private:
  ObjectFactory m_ndnFactory;
  ObjectFactory m_strategyFactory;
  ObjectFactory m_contentStoreFactory;
  ObjectFactory m_pitFactory;
  ObjectFactory m_fibFactory;
  ObjectFactory m_fib2Factory;

  bool     m_limitsEnabled;
  Time     m_avgRtt;
  uint32_t m_avgContentObjectSize;
  uint32_t m_avgInterestSize;

};

} // namespace ndn
} // namespace ns3

#endif /* NDN_BCUBE_STACK_HELPER_H */
