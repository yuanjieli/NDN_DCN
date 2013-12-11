/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef _NDN_FIB_ENTRY_H_
#define	_NDN_FIB_ENTRY_H_

#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-limits.h"
#include "ns3/traced-value.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

//parameters for weight update
#define UPDATE_INTERVAL 1
#define SHOW_RATE_INTERVAL 1
#define ALPHA 1/16.0	//smoothed counter

namespace ns3 {
namespace ndn {

class Name;
typedef Name NameComponents;

namespace fib {

/**
 * \ingroup ndn
 * \brief Structure holding various parameters associated with a (FibEntry, Face) tuple
 */
class FaceMetric
{
public:
  /**
   * @brief Color codes for FIB face status
   */
  enum Status { NDN_FIB_GREEN = 1,
                NDN_FIB_YELLOW = 2,
                NDN_FIB_RED = 3 };
public:
  /**
   * \brief Metric constructor
   *
   * \param face Face for which metric
   * \param cost Initial value for routing cost
   */
  FaceMetric (Ptr<Face> face, int32_t cost)
    : m_face (face)
    , m_status (NDN_FIB_YELLOW)
    , m_routingCost (cost)
    , m_sRtt   (Seconds (0))
    , m_rttVar (Seconds (0))
    , m_realDelay (Seconds (0))
    , m_nack (0)	
    , m_data_in (0) 
    , m_data_ce (0) 
    , m_nack_old (0)
    , m_data_in_old (0)
    , m_data_ce_old (0)   
    , m_fraction (1) //initially arbitrary large number. Will be updated later
    , m_sharing_metric (1)
    , m_interest_count (0)
  { }
 
  /**
   * \brief Comparison operator used by boost::multi_index::identity<>
   */
  bool
  operator< (const FaceMetric &fm) const { return *m_face < *fm.m_face; } // return identity of the face

  /**
   * @brief Comparison between FaceMetric and Face
   */
  bool
  operator< (const Ptr<Face> &face) const { return *m_face < *face; }

  /**
   * @brief Return Face associated with FaceMetric
   */
  Ptr<Face>
  GetFace () const { return m_face; }

  /**
   * \brief Recalculate smoothed RTT and RTT variation
   * \param rttSample RTT sample
   */
  void
  UpdateRtt (const Time &rttSample);

  /**
   * @brief Get current status of FIB entry
   */
  Status
  GetStatus () const
  {
    return m_status;
  }

  /**
   * @brief Set current status of FIB entry
   */
  void
  SetStatus (Status status)
  {
    m_status.Set (status);
  }

  /**
   * @brief Get current routing cost
   */
  int32_t
  GetRoutingCost () const
  {
    return m_routingCost;
  }

  /**
   * @brief Set routing cost
   */
  void
  SetRoutingCost (int32_t routingCost)
  {
    m_routingCost = routingCost;
  }

  /**
   * @brief Get real propagation delay to the producer, calculated based on NS-3 p2p link delays
   */
  Time
  GetRealDelay () const
  {
    return m_realDelay;
  }

  /**
   * @brief Set real propagation delay to the producer, calculated based on NS-3 p2p link delays
   */
  void
  SetRealDelay (Time realDelay)
  {
    m_realDelay = realDelay;
  }

  /**
   * @brief Get direct access to status trace
   */
  TracedValue<Status> &
  GetStatusTrace ()
  {
    return m_status;
  }
  
  void
  IncreaseNack()
  {

  	m_nack++;
  }
  
  void
  SetNack(double rhs)
  {
  	m_nack = rhs;
  }
  
  double
  GetNack() const
  {
  	//return m_nack_old+1;
  	return m_nack+1;
  }
  
  double
  GetNackOld() const
  {
  	return m_nack_old;
  }
  
  void
  IncreaseDataIn()
  {
  	m_data_in++;
  }
  
  void
  SetDataIn(double rhs)
  {
  	m_data_in = rhs;
  }
  
  double
  GetDataIn() const
  {
  	return m_data_in_old+1;
  }
  
  void
  IncreaseDataCE()
  {
  	m_data_ce++;
  }
  
  void
  SetDataCE(double rhs)
  {
  	m_data_ce = rhs;
  }
  
  double
  GetDataCE() const
  {
  	return m_data_ce_old+1;
  }
  
  double
  GetFraction() const
  {
  	return m_fraction;
  }
  
  void
  SetFraction(double rhs) //if 50%, rhs=50
 	{
 		m_fraction = rhs;
 	}
  
  double
  GetSharingMetric() const
  {
  	
  	
  	return m_sharing_metric;
  }
  
  void
  IncreaseInterest()
  {
  	m_interest_count++;
  }
  
  uint32_t 
  GetInterest() const
  {
  	return m_interest_count;
  }
 
  
  void 
  ResetCounter ()
  {
  	m_data_in_old = ALPHA*m_data_in+(1-ALPHA)*m_data_in_old;
  	m_data_ce_old = ALPHA*m_data_ce+(1-ALPHA)*m_data_ce_old;
  	m_nack_old = ALPHA*m_nack+(1-ALPHA)*m_nack_old;
  	
  	
  
  	//m_sharing_metric = (m_data_in_old+1)*(m_data_in_old+1)/(double)((m_data_ce_old+1)*(m_nack_old+1)); 
  	//m_sharing_metric = (m_data_in_old+1)/(double)(m_nack_old+1);  
  	//m_sharing_metric = (double)(m_nack_old)*100.0/m_sharing_metric; 
  	
  	
  	m_data_in = 0;
  	m_data_ce = 0;
  	m_nack = 0;
  	
  	m_interest_count = 0;
  	
  }

private:
  friend std::ostream& operator<< (std::ostream& os, const FaceMetric &metric);

private:
  Ptr<Face> m_face; ///< Face

  TracedValue<Status> m_status; ///< \brief Status of the next hop:
				///<		- NDN_FIB_GREEN
				///<		- NDN_FIB_YELLOW
				///<		- NDN_FIB_RED

  int32_t m_routingCost; ///< \brief routing protocol cost (interpretation of the value depends on the underlying routing protocol)

  Time m_sRtt;         ///< \brief smoothed round-trip time
  Time m_rttVar;       ///< \brief round-trip time variation

  Time m_realDelay;    ///< \brief real propagation delay to the producer, calculated based on NS-3 p2p link delays

	double m_nack;		 ///< \brief nack counter
	double m_data_in;  ///< \brief incoming data counter
	double m_data_ce;  ///< \brief incoming marked data counter
	//the following variables are used for storing counters last round
	double m_nack_old;
	double m_data_in_old;
	double m_data_ce_old;

  double m_fraction;				///< fraction of traffic this face can forward(%)
	double m_sharing_metric;	///< used for calculating m_fraction
	
	uint32_t m_interest_count;			///< used for debug
};

/// @cond include_hidden
class i_face {};
class i_metric {};
class i_nth {};
/// @endcond


/**
 * \ingroup ndn
 * \brief Typedef for indexed face container of Entry
 *
 * Currently, there are 2 indexes:
 * - by face (used to find record and update metric)
 * - by metric (face ranking)
 * - random access index (for fast lookup on nth face). Order is
 *   maintained manually to be equal to the 'by metric' order
 */
struct FaceMetricContainer
{
  /// @cond include_hidden
  typedef boost::multi_index::multi_index_container<
    FaceMetric,
    boost::multi_index::indexed_by<
      // For fast access to elements using Face
      boost::multi_index::ordered_unique<
        boost::multi_index::tag<i_face>,
        boost::multi_index::const_mem_fun<FaceMetric,Ptr<Face>,&FaceMetric::GetFace>
      >,

      // List of available faces ordered by (status, m_routingCost)
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<i_metric>,
        boost::multi_index::composite_key<
          FaceMetric,
          boost::multi_index::const_mem_fun<FaceMetric,FaceMetric::Status,&FaceMetric::GetStatus>,
          boost::multi_index::const_mem_fun<FaceMetric,int32_t,&FaceMetric::GetRoutingCost>
        >
      >,

      // To optimize nth candidate selection (sacrifice a little bit space to gain speed)
      boost::multi_index::random_access<
        boost::multi_index::tag<i_nth>
      >
    >
   > type;
  /// @endcond
};

/**
 * \ingroup ndn
 * \brief Structure for FIB table entry, holding indexed list of
 *        available faces and their respective metrics
 */
class Entry : public Object
{
public:
  typedef Entry base_type;

public:
  class NoFaces {}; ///< @brief Exception class for the case when FIB entry is not found

  /**
   * \brief Constructor
   * \param prefix smart pointer to the prefix for the FIB entry
   */
  Entry (const Ptr<const Name> &prefix)
  : m_prefix (prefix)
  , m_needsProbing (false)
  , m_inited (false)
  , m_data (0)
  {
  	Simulator::Schedule (Seconds (0.001), &Entry::ShowRate, this);
  	Simulator::Schedule (Seconds (0.001), &Entry::ResetCount, this);
  	
  }

  /**
   * \brief Update status of FIB next hop
   * \param status Status to set on the FIB entry
   */
  void UpdateStatus (Ptr<Face> face, FaceMetric::Status status);

  /**
   * \brief Add or update routing metric of FIB next hop
   *
   * Initial status of the next hop is set to YELLOW
   */
  void AddOrUpdateRoutingMetric (Ptr<Face> face, int32_t metric);

  /**
   * \brief Set real delay to the producer
   */
  void
  SetRealDelayToProducer (Ptr<Face> face, Time delay);

  /**
   * @brief Invalidate face
   *
   * Set routing metric on all faces to max and status to RED
   */
  void
  Invalidate ();

  /**
   * @brief Update RTT averages for the face
   */
  void
  UpdateFaceRtt (Ptr<Face> face, const Time &sample);

  /**
   * \brief Get prefix for the FIB entry
   */
  const Name&
  GetPrefix () const { return *m_prefix; }

  /**
   * \brief Find "best route" candidate, skipping `skip' first candidates (modulo # of faces)
   *
   * throws Entry::NoFaces if m_faces.size()==0
   */
  const FaceMetric &
  FindBestCandidate (uint32_t skip = 0) const;

  /**
   * @brief Remove record associated with `face`
   */
  void
  RemoveFace (const Ptr<Face> &face)
  {
    m_faces.erase (face);
  }
  
  void
  IncreaseData ()
  {
  		m_data ++;
  }
  
  uint32_t
  GetData ()
  {
  	return m_data;
  }
	
private:
  friend std::ostream& operator<< (std::ostream& os, const Entry &entry);
  	
  void ResetCount();
  void ShowRate();

public:
  Ptr<const Name> m_prefix; ///< \brief Prefix of the FIB entry
  FaceMetricContainer::type m_faces; ///< \brief Indexed list of faces

  bool m_needsProbing;      ///< \brief flag indicating that probing should be performed
	bool m_inited;					///< whether it is initialized
	
	uint32_t m_data;				///< brief used for measuring real throughput
};

std::ostream& operator<< (std::ostream& os, const Entry &entry);
std::ostream& operator<< (std::ostream& os, const FaceMetric &metric);

} // namespace fib
} // namespace ndn
} // namespace ns3

#endif // _NDN_FIB_ENTRY_H_
