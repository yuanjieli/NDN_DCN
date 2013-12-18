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
#include "../model/ndn-l2-protocol.h"
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
{
  
	m_L2Factory. SetTypeId ("ns3::ndn::L2Protocol");
  
}

SwitchStackHelper::~SwitchStackHelper ()
{
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

  if (node->GetObject<L2Protocol> () != 0)
    {
      NS_FATAL_ERROR ("SwitchStackHelper::Install (): Installing "
                      "a SwitchStack to a node with an existing Ndn object");
      return 0;
    }

	// Create L2Protocol
	Ptr<L2Protocol> L2 = m_L2Factory.Create<L2Protocol> ();
	
  // Aggregate L2Protocol on node
  node->AggregateObject (L2);

  for (uint32_t index=0; index < node->GetNDevices (); index++)
    {
      Ptr<NetDevice> device = node->GetDevice (index);
      // This check does not make sense: LoopbackNetDevice is installed only if IP stack is installed,
      // Normally, ndnSIM works without IP stack, so no reason to check
      // if (DynamicCast<LoopbackNetDevice> (device) != 0)
      //   continue; // don't create face for a LoopbackNetDevice

      Ptr<NetDeviceFace> face = PointToPointNetDevice(node, L2, device);

      if (face == 0)
        {
        	NS_LOG_DEBUG("Empty Faces in SwitchStackHelper!");
          return 0;
        }

      face->SetUp ();
      faces->Add (face);
    }

  return faces;
}


Ptr<NetDeviceFace>
SwitchStackHelper::PointToPointNetDevice (Ptr<Node> node, Ptr<L2Protocol> L2, Ptr<NetDevice> device) const
{
  NS_LOG_DEBUG ("Creating point-to-point NetDeviceFace on node " << node->GetId ());

	//NOTE: we need two separate links here
  Ptr<NetDeviceFace> uploadface = CreateObject<NetDeviceFace> (node, device);
  Ptr<NetDeviceFace> downloadface = CreateObject<NetDeviceFace> (node, device);

 
  L2->AddFace (uploadface, downloadface);
  
  return uploadface;
}


Ptr<FaceContainer>
SwitchStackHelper::Install (const std::string &nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}



} // namespace ndn
} // namespace ns3
