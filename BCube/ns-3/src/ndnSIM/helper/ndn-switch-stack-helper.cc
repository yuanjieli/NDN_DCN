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
 * Author:  Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *          Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/names.h"
#include "ns3/packet-socket-factory.h"
#include "ns3/config.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/callback.h"
#include "ns3/node.h"
#include "ns3/core-config.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/callback.h"

#include "../model/ndn-net-device-face.h"
#include "../model/ndn-l3-protocol.h"

#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-fib2.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-content-store.h"

#include "ns3/node-list.h"
// #include "ns3/loopback-net-device.h"

#include "ns3/data-rate.h"

#include "ndn-face-container.h"
#include "ndn-switch-stack-helper.h"

#include <limits>
#include <map>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.SwitchStackHelper");

namespace ns3 {
namespace ndn {

SwitchStackHelper::SwitchStackHelper ()
  : m_limitsEnabled (false)
  , m_needSetDefaultRoutes (false)
{
  m_ndnFactory.         SetTypeId ("ns3::ndn::L3Protocol");
  m_strategyFactory.    SetTypeId ("ns3::ndn::fw::Flooding");
  m_contentStoreFactory.SetTypeId ("ns3::ndn::cs::Lru");
  m_fibFactory.         SetTypeId ("ns3::ndn::fib::Default");
  m_fib2Factory.				SetTypeId ("ns3::ndn::fib2::Default");
  m_pitFactory.         SetTypeId ("ns3::ndn::pit::Persistent");

  m_netDeviceCallbacks.push_back (std::make_pair (PointToPointNetDevice::GetTypeId (), MakeCallback (&SwitchStackHelper::PointToPointNetDeviceCallback, this)));
  // default callback will be fired if non of others callbacks fit or did the job
}

SwitchStackHelper::~SwitchStackHelper ()
{
}


void
SwitchStackHelper::SetDefaultRoutes (bool needSet)
{
  NS_LOG_FUNCTION (this << needSet);
  m_needSetDefaultRoutes = needSet;
}

Ptr<FaceContainer>
SwitchStackHelper::Install (const NodeContainer &c) const
{
  Ptr<FaceContainer> faces = Create<FaceContainer> ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      faces->AddAll (Install (*i));
    }
  return faces;
}

Ptr<FaceContainer>
SwitchStackHelper::InstallAll () const
{
  return Install (NodeContainer::GetGlobal ());
}

Ptr<FaceContainer>
SwitchStackHelper::Install (Ptr<Node> node) const
{
  // NS_ASSERT_MSG (m_forwarding, "SetForwardingHelper() should be set prior calling Install() method");
  Ptr<FaceContainer> faces = Create<FaceContainer> ();

  if (node->GetObject<L3Protocol> () != 0)
    {
      NS_FATAL_ERROR ("SwitchStackHelper::Install (): Installing "
                      "a NdnStack to a node with an existing Ndn object");
      return 0;
    }

  // Create L3Protocol
  Ptr<L3Protocol> ndn = m_ndnFactory.Create<L3Protocol> ();

  // Create and aggregate FIB
  Ptr<Fib> fib = m_fibFactory.Create<Fib> ();
  ndn->AggregateObject (fib);
  
  // Create and aggregate FIB2
  Ptr<Fib2> fib2 = m_fib2Factory.Create<Fib2> ();
  ndn->AggregateObject (fib2);

  // Create and aggregate PIT
  ndn->AggregateObject (m_pitFactory.Create<Pit> ());

  // Create and aggregate forwarding strategy
  ndn->AggregateObject (m_strategyFactory.Create<ForwardingStrategy> ());

  // Create and aggregate content store
  ndn->AggregateObject (m_contentStoreFactory.Create<ContentStore> ());

  // Aggregate L3Protocol on node
  node->AggregateObject (ndn);

  for (uint32_t index=0; index < node->GetNDevices (); index++)
    {
      Ptr<NetDevice> device = node->GetDevice (index);
      // This check does not make sense: LoopbackNetDevice is installed only if IP stack is installed,
      // Normally, ndnSIM works without IP stack, so no reason to check
      // if (DynamicCast<LoopbackNetDevice> (device) != 0)
      //   continue; // don't create face for a LoopbackNetDevice

      Ptr<NetDeviceFace> face;

      for (std::list< std::pair<TypeId, NetDeviceFaceCreateCallback> >::const_iterator item = m_netDeviceCallbacks.begin ();
           item != m_netDeviceCallbacks.end ();
           item++)
        {
          if (device->GetInstanceTypeId () == item->first ||
              device->GetInstanceTypeId ().IsChildOf (item->first))
            {
              face = item->second (node, ndn, device);
              if (face != 0)
                break;
            }
        }
      if (face == 0)
        {
          face = DefaultNetDeviceCallback (node, ndn, device);
        }

      if (m_needSetDefaultRoutes)
        {
          // default route with lowest priority possible
          AddRoute (node, "/", StaticCast<Face> (face), std::numeric_limits<int32_t>::max ());
        }

      face->SetUp ();
      faces->Add (face);
    }

  return faces;
}




Ptr<NetDeviceFace>
SwitchStackHelper::DefaultNetDeviceCallback (Ptr<Node> node, Ptr<L3Protocol> ndn, Ptr<NetDevice> netDevice) const
{
  NS_LOG_DEBUG ("Creating default NetDeviceFace on node " << node->GetId ());

  Ptr<NetDeviceFace> face = CreateObject<NetDeviceFace> (node, netDevice);

  ndn->AddFace (face);
  NS_LOG_LOGIC ("Node " << node->GetId () << ": added NetDeviceFace as face #" << *face);

  return face;
}

Ptr<NetDeviceFace>
SwitchStackHelper::PointToPointNetDeviceCallback (Ptr<Node> node, Ptr<L3Protocol> ndn, Ptr<NetDevice> device) const
{
  NS_LOG_DEBUG ("Creating point-to-point NetDeviceFace on node " << node->GetId ());

  Ptr<NetDeviceFace> face = CreateObject<NetDeviceFace> (node, device);

  ndn->AddFace (face);
  NS_LOG_LOGIC ("Node " << node->GetId () << ": added NetDeviceFace as face #" << *face);

  if (m_limitsEnabled)
    {
      Ptr<Limits> limits = face->GetObject<Limits> ();
      if (limits == 0)
        {
          NS_FATAL_ERROR ("Limits are enabled, but the selected forwarding strategy does not support limits. Please revise your scenario");
          exit (1);
        }

      NS_LOG_INFO ("Limits are enabled");
      Ptr<PointToPointNetDevice> p2p = DynamicCast<PointToPointNetDevice> (device);
      if (p2p != 0)
        {
          // Setup bucket filtering
          // Assume that we know average data packet size, and this size is equal default size
          // Set maximum buckets (averaging over 1 second)

          DataRateValue dataRate; device->GetAttribute ("DataRate", dataRate);
          TimeValue linkDelay;   device->GetChannel ()->GetAttribute ("Delay", linkDelay);

          NS_LOG_INFO("DataRate for this link is " << dataRate.Get());

          double maxInterestPackets = 1.0  * dataRate.Get ().GetBitRate () / 8.0 / (m_avgContentObjectSize + m_avgInterestSize);
          // NS_LOG_INFO ("Max packets per second: " << maxInterestPackets);
          // NS_LOG_INFO ("Max burst: " << m_avgRtt.ToDouble (Time::S) * maxInterestPackets);
          NS_LOG_INFO ("MaxLimit: " << (int)(m_avgRtt.ToDouble (Time::S) * maxInterestPackets));

          // Set max to BDP
          limits->SetLimits (maxInterestPackets, m_avgRtt.ToDouble (Time::S));
          limits->SetLinkDelay (linkDelay.Get ().ToDouble (Time::S));
        }
    }

  return face;
}


Ptr<FaceContainer>
SwitchStackHelper::Install (const std::string &nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}


void
SwitchStackHelper::AddRoute (Ptr<Node> node, const std::string &prefix, Ptr<Face> face, int32_t metric)
{
  NS_LOG_LOGIC ("[" << node->GetId () << "]$ route add " << prefix << " via " << *face << " metric " << metric);

  Ptr<Fib>  fib  = node->GetObject<Fib> ();

  NameValue prefixValue;
  prefixValue.DeserializeFromString (prefix, MakeNameChecker ());
  fib->Add (prefixValue.Get (), face, metric);
}

void
SwitchStackHelper::AddRoute (Ptr<Node> node, const std::string &prefix, uint32_t faceId, int32_t metric)
{
  Ptr<L3Protocol>     ndn = node->GetObject<L3Protocol> ();
  NS_ASSERT_MSG (ndn != 0, "Ndn stack should be installed on the node");

  Ptr<Face> face = ndn->GetFace (faceId);
  NS_ASSERT_MSG (face != 0, "Face with ID [" << faceId << "] does not exist on node [" << node->GetId () << "]");

  AddRoute (node, prefix, face, metric);
}

void
SwitchStackHelper::AddRoute (const std::string &nodeName, const std::string &prefix, uint32_t faceId, int32_t metric)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  NS_ASSERT_MSG (node != 0, "Node [" << nodeName << "] does not exist");

  Ptr<L3Protocol>     ndn = node->GetObject<L3Protocol> ();
  NS_ASSERT_MSG (ndn != 0, "Ndn stack should be installed on the node");

  Ptr<Face> face = ndn->GetFace (faceId);
  NS_ASSERT_MSG (face != 0, "Face with ID [" << faceId << "] does not exist on node [" << nodeName << "]");

  AddRoute (node, prefix, face, metric);
}

void
SwitchStackHelper::AddRoute (Ptr<Node> node, const std::string &prefix, Ptr<Node> otherNode, int32_t metric)
{
  for (uint32_t deviceId = 0; deviceId < node->GetNDevices (); deviceId ++)
    {
      Ptr<PointToPointNetDevice> netDevice = DynamicCast<PointToPointNetDevice> (node->GetDevice (deviceId));
      if (netDevice == 0)
        continue;

      Ptr<Channel> channel = netDevice->GetChannel ();
      if (channel == 0)
        continue;

      if (channel->GetDevice (0)->GetNode () == otherNode ||
          channel->GetDevice (1)->GetNode () == otherNode)
        {
          Ptr<L3Protocol> ndn = node->GetObject<L3Protocol> ();
          NS_ASSERT_MSG (ndn != 0, "Ndn stack should be installed on the node");

          Ptr<Face> face = ndn->GetFaceByNetDevice (netDevice);
          NS_ASSERT_MSG (face != 0, "There is no face associated with the p2p link");

          AddRoute (node, prefix, face, metric);

          return;
        }
    }

  NS_FATAL_ERROR ("Cannot add route: Node# " << node->GetId () << " and Node# " << otherNode->GetId () << " are not connected");
}

void
SwitchStackHelper::AddRoute (const std::string &nodeName, const std::string &prefix, const std::string &otherNodeName, int32_t metric)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  NS_ASSERT_MSG (node != 0, "Node [" << nodeName << "] does not exist");

  Ptr<Node> otherNode = Names::Find<Node> (otherNodeName);
  NS_ASSERT_MSG (otherNode != 0, "Node [" << otherNodeName << "] does not exist");

  AddRoute (node, prefix, otherNode, metric);
}


} // namespace ndn
} // namespace ns3
