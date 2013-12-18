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

#ifndef NDN_L2_PROTOCOL_H
#define NDN_L2_PROTOCOL_H

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
 * This is a simplified modular implementation of L2 protocol
 */
class L2Protocol :
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
  L2Protocol();
  virtual ~L2Protocol ();

  /**
   * \brief Add face to swith stack
   *
   * WARNING: this function would be called by ndn-switch-stack-helper
   * For each NetDevice, there would be two faces associated with it: upload face and download face
   * That's the reason we have two parameters here
   * upload_face: from server to switch
   * download_face: from switch to server
   * Each NetDevice would only be counted once
   */
  virtual uint32_t
  AddFace (const Ptr<Face> &upload_face, const Ptr<Face> &download_face); 
  
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
  /*virtual void
  RemoveFace (Ptr<Face> face);*/

  /**
   * \brief Get face for NetDevice
   */
  virtual Ptr<Face>
  GetFaceByNetDevice (Ptr<NetDevice> netDevice) const;

  
private:
  void
  Receive (const Ptr<Face> &face, const Ptr<const Packet> &p);

protected:
  virtual void DoDispose (void); ///< @brief Do cleanup

  /**
   * This function will notify other components connected to the node that a new stack member is now connected
   */
  virtual void NotifyNewAggregate ();

private:
  L2Protocol(const L2Protocol &); ///< copy constructor is disabled
  L2Protocol &operator = (const L2Protocol &); ///< copy operator is disabled
  
private:
  uint32_t m_faceCounter; ///< \brief counter of faces. Increased every time a new face is added to the stack
  FaceList m_uploadfaces; ///< \brief list of upload faces that belongs to switch
	FaceList m_downloadfaces; ///< \brief list of download faces that belongs to switch 
  
  // These objects are aggregated, but for optimization, get them here
  Ptr<Node> m_node; ///< \brief node on which ndn stack is installed
};

} // namespace ndn
} // namespace ns3

#endif /* NDN_L3_PROTOCOL_H */
