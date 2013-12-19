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

#ifndef NDN_BCUBE_L3_PROTOCOL_H
#define NDN_BCUBE_L3_PROTOCOL_H

#include <list>
#include <vector>

#include "ns3/ptr.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"

namespace ns3 {

class Packet;
class NetDevice;
class Node;
class Header;

namespace ndn {

class Face;
class ForwardingStrategy;
class Interest;
class ContentObject;

typedef Interest InterestHeader;
typedef ContentObject ContentObjectHeader;

/**
 * \defgroup ndn ndnSIM: NDN simulation module
 *
 * This is a simplified modular implementation of NDN protocol
 */
/**
 * \ingroup ndn
 * \brief Implementation network-layer of NDN stack
 *
 * This class defines the API to manipulate the following aspects of
 * the NDN stack implementation:
 * -# register a face (Face-derived object) for use by the NDN
 *    layer
 * 
 * Each Face-derived object has conceptually a single NDN
 * interface associated with it.
 *
 * In addition, this class defines NDN packet coding constants
 *
 * \see Face, ForwardingStrategy
 */
class BCubeL3Protocol :
    public Object
{
public:
  typedef std::vector<Ptr<Face> > FaceList;

  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId GetTypeId ();

  static const uint16_t ETHERNET_FRAME_TYPE; ///< \brief Ethernet Frame Type of Ndn
  // static const uint16_t IP_PROTOCOL_TYPE;    ///< \brief IP protocol type of Ndn
  // static const uint16_t UDP_PORT;            ///< \brief UDP port of Ndn

  /**
   * \brief Default constructor. Creates an empty stack without forwarding strategy set
   */
  BCubeL3Protocol();
  virtual ~BCubeL3Protocol ();

  /**
   * \brief Add face to Ndn stack
   *
   * \param face smart pointer to NdnFace-derived object
   * (NdnLocalFace, NdnNetDeviceFace, NdnUdpFace) \returns the
   * index of the Ndn interface added.
   * 
   * \see NdnLocalFace, NdnNetDeviceFace, NdnUdpFace
   */
  virtual uint32_t
  AddFace (const Ptr<Face> &uploadface, const Ptr<Face> &downloadface);
  
  virtual uint32_t
  AddAppFace (const Ptr<Face> face);
  
  /**
   * \brief Get current number of faces added to Ndn stack
   *
   * \returns the number of faces
   */
  virtual uint32_t
  GetNFaces () const;

  /**
   * \brief Get face by face index
   * \param face The face number (number in face list)
   * \returns The NdnFace associated with the Ndn face number.
   */
  virtual Ptr<Face>
  GetUploadFace (uint32_t face) const;
  
  virtual Ptr<Face>
  GetDownloadFace (uint32_t face) const;
  
  /**
   * \brief Get face by face ID
   * \param face The face ID number
   * \returns The NdnFace associated with the Ndn face number.
   */
  virtual Ptr<Face>
  GetFaceById (uint32_t face) const;

  /**
   * \brief Remove face from ndn stack (remove callbacks)
   */
  virtual void
  RemoveFace (Ptr<Face> face);

  /**
   * \brief Get face for NetDevice
   */
  virtual Ptr<Face>
  GetUploadFaceByNetDevice (Ptr<NetDevice> netDevice) const;
  
  virtual Ptr<Face>
  GetDownloadFaceByNetDevice (Ptr<NetDevice> netDevice) const;

  static uint64_t
  GetInterestCounter ();

  static uint64_t
  GetDataCounter ();
  
private:
  void
  Receive (const Ptr<Face> &face, const Ptr<const Packet> &p);

protected:
  virtual void DoDispose (void); ///< @brief Do cleanup

  /**
   * This function will notify other components connected to the node that a new stack member is now connected
   * This will be used to notify Layer 3 protocol of layer 4 protocol stack to connect them together.
   */
  virtual void NotifyNewAggregate ();

private:
  BCubeL3Protocol(const BCubeL3Protocol &); ///< copy constructor is disabled
  BCubeL3Protocol &operator = (const BCubeL3Protocol &); ///< copy operator is disabled
  
private:
  uint32_t m_faceCounter; ///< \brief counter of faces. Increased every time a new face is added to the stack
  FaceList m_uploadfaces; ///< \brief list of faces that belongs to ndn stack on this node
  FaceList m_downloadfaces;

  static uint64_t s_interestCounter;
  static uint64_t s_dataCounter;
  
  // These objects are aggregated, but for optimization, get them here
  Ptr<Node> m_node; ///< \brief node on which ndn stack is installed
  Ptr<ForwardingStrategy> m_forwardingStrategy; ///< \brief smart pointer to the selected forwarding strategy
};

} // namespace ndn
} // namespace ns3

#endif /* NDN_BCUBE_L3_PROTOCOL_H */
