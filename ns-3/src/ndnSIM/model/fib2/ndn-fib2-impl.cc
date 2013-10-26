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

#include "ndn-fib2-impl.h"

#include "ns3/ndn-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-forwarding-strategy.h"

#include "ns3/node.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/log.h"

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.fib2.Fib2Impl");

namespace ns3 {
namespace ndn {
namespace fib2 {

NS_OBJECT_ENSURE_REGISTERED (Fib2Impl);

TypeId 
Fib2Impl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fib2::Default") // cheating ns3 object system
    .SetParent<Fib2> ()
    .SetGroupName ("Ndn")
    .AddConstructor<Fib2Impl> ()
  ;
  return tid;
}

Fib2Impl::Fib2Impl ()
{
}

void
Fib2Impl::NotifyNewAggregate ()
{
  Object::NotifyNewAggregate ();
}

void 
Fib2Impl::DoDispose (void)
{
  clear ();
  Object::DoDispose ();
}


Ptr<Entry>
Fib2Impl::LongestPrefixMatch (const Interest &interest)
{
  super::iterator item = super::longest_prefix_match (interest.GetName ());
  // @todo use predicate to search with exclude filters

  if (item == super::end ())
    return 0;
  else
    return item->payload ();
}

Ptr<fib2::Entry>
Fib2Impl::Find (const Name &prefix)
{
  super::iterator item = super::find_exact (prefix);

  if (item == super::end ())
    return 0;
  else
    return item->payload ();
}


Ptr<Entry>
Fib2Impl::Add (const Name &prefix, Ptr<Face> face, int32_t metric)
{
  return Add (Create<Name> (prefix), face, metric);
}
  
Ptr<Entry>
Fib2Impl::Add (const Ptr<const Name> &prefix, Ptr<Face> face, int32_t metric)
{
  NS_LOG_FUNCTION (this->GetObject<Node> ()->GetId () << boost::cref(*prefix) << boost::cref(*face) << metric);

  // will add entry if doesn't exists, or just return an iterator to the existing entry
  std::pair< super::iterator, bool > result = super::insert (*prefix, 0);
  if (result.first != super::end ())
    {
      if (result.second)
        {
            Ptr<EntryImpl> newEntry = Create<EntryImpl> (prefix);
            newEntry->SetTrie (result.first);
            result.first->set_payload (newEntry);
        }
  
      super::modify (result.first,
                     ll::bind (&Entry::AddOrUpdateRoutingMetric, ll::_1, face, metric));

      if (result.second)
        {
          // notify forwarding strategy about new FIB entry
          NS_ASSERT (this->GetObject<ForwardingStrategy> () != 0);
          this->GetObject<ForwardingStrategy> ()->DidAddFib2Entry (result.first->payload ());
        }
      
      return result.first->payload ();
    }
  else
  {
  	NS_LOG_UNCOND("Fib2Impl::Add returns 0!");
    return 0;
  }
}

void
Fib2Impl::Remove (const Ptr<const Name> &prefix)
{
  NS_LOG_FUNCTION (this->GetObject<Node> ()->GetId () << boost::cref(*prefix));

  super::iterator fib2Entry = super::find_exact (*prefix);
  if (fib2Entry != super::end ())
    {
      // notify forwarding strategy about soon be removed FIB entry
      NS_ASSERT (this->GetObject<ForwardingStrategy> () != 0);
      this->GetObject<ForwardingStrategy> ()->WillRemoveFib2Entry (fib2Entry->payload ());

      super::erase (fib2Entry);
    }
  // else do nothing
}

// void
// FibImpl::Invalidate (const Ptr<const Name> &prefix)
// {
//   NS_LOG_FUNCTION (this->GetObject<Node> ()->GetId () << boost::cref(*prefix));

//   super::iterator foundItem, lastItem;
//   bool reachLast;
//   boost::tie (foundItem, reachLast, lastItem) = super::getTrie ().find (*prefix);
  
//   if (!reachLast || lastItem->payload () == 0)
//     return; // nothing to invalidate

//   super::modify (lastItem,
//                  ll::bind (&Entry::Invalidate, ll::_1));
// }

void
Fib2Impl::InvalidateAll ()
{
  NS_LOG_FUNCTION (this->GetObject<Node> ()->GetId ());

  super::parent_trie::recursive_iterator item (super::getTrie ());
  super::parent_trie::recursive_iterator end (0);
  for (; item != end; item++)
    {
      if (item->payload () == 0) continue;

      super::modify (&(*item),
                     ll::bind (&Entry::Invalidate, ll::_1));
    }
}

void
Fib2Impl::RemoveFace (super::parent_trie &item, Ptr<Face> face)
{
  if (item.payload () == 0) return;
  NS_LOG_FUNCTION (this);

  super::modify (&item,
                 ll::bind (&Entry::RemoveFace, ll::_1, face));
}

void
Fib2Impl::RemoveFromAll (Ptr<Face> face)
{
  NS_LOG_FUNCTION (this);

  std::for_each (super::parent_trie::recursive_iterator (super::getTrie ()),
                 super::parent_trie::recursive_iterator (0), 
                 ll::bind (&Fib2Impl::RemoveFace,
                           this, ll::_1, face));

  super::parent_trie::recursive_iterator trieNode (super::getTrie ());
  super::parent_trie::recursive_iterator end (0);
  for (; trieNode != end; trieNode++)
    {
      if (trieNode->payload () == 0) continue;
      
      if (trieNode->payload ()->m_faces.size () == 0)
        {
          // notify forwarding strategy about soon be removed FIB entry
          NS_ASSERT (this->GetObject<ForwardingStrategy> () != 0);
          this->GetObject<ForwardingStrategy> ()->WillRemoveFib2Entry (trieNode->payload ());
          
          trieNode = super::parent_trie::recursive_iterator (trieNode->erase ());
        }
    }
}

void
Fib2Impl::Print (std::ostream &os) const
{
  // !!! unordered_set imposes "random" order of item in the same level !!!
  super::parent_trie::const_recursive_iterator item (super::getTrie ());
  super::parent_trie::const_recursive_iterator end (0);
  for (; item != end; item++)
    {
      if (item->payload () == 0) continue;

      os << item->payload ()->GetPrefix () << "\t" << *item->payload () << "\n";
    }
}

uint32_t
Fib2Impl::GetSize () const
{
  return super::getPolicy ().size ();
}

Ptr<const Entry>
Fib2Impl::Begin () const
{
  super::parent_trie::const_recursive_iterator item (super::getTrie ());
  super::parent_trie::const_recursive_iterator end (0);
  for (; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}

Ptr<const Entry>
Fib2Impl::End () const
{
  return 0;
}

Ptr<const Entry>
Fib2Impl::Next (Ptr<const Entry> from) const
{
  if (from == 0) return 0;
  
  super::parent_trie::const_recursive_iterator item (*StaticCast<const EntryImpl> (from)->to_iterator ());
  super::parent_trie::const_recursive_iterator end (0);
  for (item++; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}

Ptr<Entry>
Fib2Impl::Begin ()
{
  super::parent_trie::recursive_iterator item (super::getTrie ());
  super::parent_trie::recursive_iterator end (0);
  for (; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}

Ptr<Entry>
Fib2Impl::End ()
{
  return 0;
}

Ptr<Entry>
Fib2Impl::Next (Ptr<Entry> from)
{
  if (from == 0) return 0;
  
  super::parent_trie::recursive_iterator item (*StaticCast<EntryImpl> (from)->to_iterator ());
  super::parent_trie::recursive_iterator end (0);
  for (item++; item != end; item++)
    {
      if (item->payload () == 0) continue;
      break;
    }

  if (item == end)
    return End ();
  else
    return item->payload ();
}


} // namespace fib2
} // namespace ndn
} // namespace ns3
