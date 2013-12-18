/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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

#include "ndn-l2-protocol.h"

#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/object-vector.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"

#include "ns3/ndn-header-helper.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include "ns3/ndn-face.h"
#include "ns3/ndn-forwarding-strategy.h"

#include "ndn-net-device-face.h"

#include "ndn-bcube-tag.h"

#include <boost/foreach.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.L2Protocol");

namespace ns3 {
namespace ndn {

const uint16_t L2Protocol::ETHERNET_FRAME_TYPE = 0x7777;

NS_OBJECT_ENSURE_REGISTERED (L2Protocol);



TypeId
L2Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::L2Protocol")
    .SetGroupName ("ndn")
    .SetParent<Object> ()
    .AddConstructor<L2Protocol> ()   
  ;
  return tid;
}

L2Protocol::L2Protocol()
: m_faceCounter (0)
{
  NS_LOG_FUNCTION (this);
}

L2Protocol::~L2Protocol ()
{
  NS_LOG_FUNCTION (this);
}

/*
 * This method is called by AddAgregate and completes the aggregation
 * by setting the node in the ndn stack
 */
void
L2Protocol::NotifyNewAggregate ()
{
  // not really efficient, but this will work only once
  if (m_node == 0)
    {
      m_node = GetObject<Node> ();
    }  

  Object::NotifyNewAggregate ();
}

void
L2Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  for (FaceList::iterator i = m_uploadfaces.begin (); i != m_uploadfaces.end (); ++i)
    {
      *i = 0;
    }
  for (FaceList::iterator i = m_downloadfaces.begin (); i != m_downloadfaces.end (); ++i)
    {
      *i = 0;
    }
  m_uploadfaces.clear ();
  m_downloadfaces.clear ();
  m_node = 0;

  Object::DoDispose ();
}

uint32_t
L2Protocol::AddFace (const Ptr<Face> &upload_face, const Ptr<Face> &download_face)
{
	NS_ASSERT(upload_face != 0 && download_face !=0 && upload_face != download_face);
  //upload_face and download_face share the same ID, but they can be got via different functions
  upload_face->SetId (m_faceCounter); // sets a unique ID of the face. This ID serves only informational purposes
	download_face->SetId (m_faceCounter); // sets a unique ID of the face. This ID serves only informational purposes

  // ask face to register in lower-layer stack
  // L2Protocol would only receive packets from upload_face (server->switch)
  // For download_face, no need for Receive callback
  upload_face->RegisterProtocolHandler (MakeCallback (&L2Protocol::Receive, this));

  m_uploadfaces.push_back (upload_face);
  m_downloadfaces.push_back (download_face);
  m_faceCounter++;

  //m_forwardingStrategy->AddFace (face); // notify that face is added
  return (m_faceCounter-1);
}


Ptr<Face>
L2Protocol::GetUploadFace (uint32_t index) const
{
  NS_ASSERT (0 <= index && index < m_uploadfaces.size ());
  return m_uploadfaces[index];
}

Ptr<Face>
L2Protocol::GetDownloadFace (uint32_t index) const
{
  NS_ASSERT (0 <= index && index < m_downloadfaces.size ());
  return m_downloadfaces[index];
}

Ptr<Face>
L2Protocol::GetFaceById (uint32_t index) const
{
  BOOST_FOREACH (const Ptr<Face> &face, m_uploadfaces) // this function is not supposed to be called often, so linear search is fine
    {
      if (face->GetId () == index)
        return face;
    }
 BOOST_FOREACH (const Ptr<Face> &face, m_downloadfaces) // this function is not supposed to be called often, so linear search is fine
  {
    if (face->GetId () == index)
      return face;
  }
  return 0;
}

Ptr<Face>
L2Protocol::GetFaceByNetDevice (Ptr<NetDevice> netDevice) const
{
  BOOST_FOREACH (const Ptr<Face> &face, m_uploadfaces) // this function is not supposed to be called often, so linear search is fine
    {
      Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace> (face);
      if (netDeviceFace == 0) continue;

      if (netDeviceFace->GetNetDevice () == netDevice)
        return face;
    }
 BOOST_FOREACH (const Ptr<Face> &face, m_downloadfaces) // this function is not supposed to be called often, so linear search is fine
  {
    Ptr<NetDeviceFace> netDeviceFace = DynamicCast<NetDeviceFace> (face);
    if (netDeviceFace == 0) continue;

    if (netDeviceFace->GetNetDevice () == netDevice)
      return face;
  }
  return 0;
}

uint32_t
L2Protocol::GetNFaces (void) const
{
  return m_uploadfaces.size ();
}

// Callback from lower layer
void
L2Protocol::Receive (const Ptr<Face> &face, const Ptr<const Packet> &p)
{
	//This face MUST be upload face
  if (!face->IsUp ())
    return;

  NS_LOG_DEBUG (*p);

  NS_LOG_LOGIC ("Packet from face " << *face << " received on node " <<  m_node->GetId ());

  Ptr<Packet> packet = p->Copy (); // give upper layers a rw copy of the packet
  BCubeTag tag;
  packet->PeekPacketTag(tag);
  
  
}


} //namespace ndn
} //namespace ns3
