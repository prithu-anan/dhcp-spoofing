#include "rogue-dhcp-server.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/inet-socket-address.h"
#include "ns3/dhcp-header.h"
#include "ns3/ipv4.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RogueDhcpServer");
NS_OBJECT_ENSURE_REGISTERED (RogueDhcpServer);

TypeId
RogueDhcpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RogueDhcpServer")
    .SetParent<DhcpServer> ()
    .AddConstructor<RogueDhcpServer> ()
    .AddAttribute ("DefaultLease", "Default lease duration in seconds",
                   TimeValue (Seconds (3600)),
                   MakeTimeAccessor (&RogueDhcpServer::m_defaultLease),
                   MakeTimeChecker ())
    .AddAttribute ("Netmask", "Subnet mask for DHCP pool",
                   Ipv4MaskValue (Ipv4Mask ("255.255.255.0")),
                   MakeIpv4MaskAccessor (&RogueDhcpServer::m_netmask),
                   MakeIpv4MaskChecker ())
    .AddAttribute ("UseFakeAddresses", "Use fake addresses when pool is exhausted",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RogueDhcpServer::m_useFakeAddresses),
                   MakeBooleanChecker ())
    .AddAttribute ("DynamicExpansion", "Dynamically expand pool when running low",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RogueDhcpServer::m_dynamicExpansion),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpansionSize", "Number of addresses to add when expanding",
                   UintegerValue (50),
                   MakeUintegerAccessor (&RogueDhcpServer::m_expansionSize),
                   MakeUintegerChecker<uint32_t> (10, 1000))
    .AddAttribute ("StarvationLease", "Very short lease for suspected starvation attacks",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RogueDhcpServer::m_starvationLease),
                   MakeTimeChecker ());
  return tid;
}

RogueDhcpServer::RogueDhcpServer ()
{
  // Initialize pool ranges
  m_poolStart = Ipv4Address("10.0.0.100").Get ();
  m_poolEnd = Ipv4Address("10.0.0.150").Get ();
  m_fakePoolStart = Ipv4Address("10.0.0.201").Get ();
  m_fakePoolEnd = Ipv4Address("10.0.0.254").Get ();
  m_reservationLease = Seconds (10);
  
  // populate the main pool [10.0.0.100 .. 10.0.0.200]
  for (uint32_t a = m_poolStart; a <= m_poolEnd; ++a) {
    m_available.push_back (Ipv4Address (a));
  }
}

RogueDhcpServer::~RogueDhcpServer () {}

void
RogueDhcpServer::StartApplication (void)
{
  // Create our own socket for intercepting DHCP packets
  m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 67);
  m_socket->Bind (local);
  m_socket->SetRecvCallback (MakeCallback (&RogueDhcpServer::NetHandler, this));
  
  // schedule lease expiry
  m_timerEvent = Simulator::Schedule (Seconds (1.0), &RogueDhcpServer::TimerHandler, this);
}

void
RogueDhcpServer::StopApplication (void)
{
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
  
  if (m_timerEvent.IsPending ())
    {
      Simulator::Cancel (m_timerEvent);
    }
}

void
RogueDhcpServer::NetHandler (Ptr<Socket> socket)
{
  Address from;
  Ptr<Packet> packet = socket->RecvFrom (from);
  DhcpHeader header;
  if (!packet->RemoveHeader (header)) {
    return;
  }
  switch (header.GetType ()) {
    case DhcpHeader::DHCPDISCOVER:
      SendSpoofedOffer (header, from);
      break;
    case DhcpHeader::DHCPREQ:
      SendSpoofedAck (header, from);
      break;
    default:
      // Ignore other types for now
      break;
  }
}

void
RogueDhcpServer::TimerHandler (void)
{
  // decrement and expire leases
  for (auto it = m_leases.begin (); it != m_leases.end (); ) {
    it->second.second -= Seconds (1.0);
    if (it->second.second <= Seconds (0)) {
      m_available.push_back (it->second.first);
      it = m_leases.erase (it);
    } else {
      ++it;
    }
  }
  m_timerEvent = Simulator::Schedule (Seconds (1.0), &RogueDhcpServer::TimerHandler, this);
}

Ipv4Address
RogueDhcpServer::AllocateAddress (const Mac48Address &chaddr, bool isDiscover=false)
{
  auto found = m_leases.find (chaddr);
  if (found != m_leases.end ()) {
    return found->second.first;
  }
  
  // Check if this might be a starvation attack
  bool isStarvation = IsStarvationAttack (chaddr);
  
  // If we're running low on addresses and dynamic expansion is enabled
  if (m_available.size () < 10 && m_dynamicExpansion) {
    ExpandPool ();
  }
  
  if (m_available.empty ()) {
    // If we're out of real addresses, try fake addresses
    if (m_useFakeAddresses) {
      return AllocateFakeAddress (chaddr);
    }
    NS_LOG_WARN ("No addresses available");
    return Ipv4Address::GetAny ();
  }
  
  Ipv4Address addr = m_available.front ();
  m_available.erase (m_available.begin ());

  
  
  // Use shorter lease for suspected starvation attacks
  Time leaseTime = isStarvation ? m_starvationLease : isDiscover ? m_reservationLease : m_defaultLease;
  m_leases[chaddr] = std::make_pair (addr, leaseTime);
  
  // If this looks like a legitimate client, add it to our tracking
  if (!isStarvation) {
    AddLegitimateClient (chaddr);
  }
  
  return addr;
}

bool
RogueDhcpServer::IsStarvationAttack (const Mac48Address &chaddr)
{
  // Simple heuristic: if we've seen this MAC before and it's requesting again quickly,
  // or if we have many recent requests from different MACs, it might be starvation
  
  // Check if this is a known legitimate client
  if (m_legitimateClients.find (chaddr) != m_legitimateClients.end ()) {
    return false;
  }
  
  // If we have many active leases (>80% of our pool), suspect starvation
  if (m_leases.size () > (m_poolEnd - m_poolStart + 1) * 0.8) {
    return true;
  }
  
  return false;
}

void
RogueDhcpServer::ExpandPool (void)
{
  // Add more addresses to the pool by extending the range
  uint32_t newEnd = m_poolEnd + m_expansionSize;
  if (newEnd <= m_fakePoolStart) { // Don't overlap with fake pool
    for (uint32_t a = m_poolEnd + 1; a <= newEnd; ++a) {
      m_available.push_back (Ipv4Address (a));
    }
    m_poolEnd = newEnd;
    NS_LOG_INFO ("Expanded pool to " << m_poolEnd - m_poolStart + 1 << " addresses");
  }
}

Ipv4Address
RogueDhcpServer::AllocateFakeAddress (const Mac48Address &chaddr)
{
  // Allocate from fake pool (addresses that don't actually exist in the network)
  static uint32_t fakeCounter = m_fakePoolStart;
  
  // Check if this MAC already has a fake address
  auto found = m_leases.find (chaddr);
  if (found != m_leases.end ()) {
    return found->second.first;
  }
  
  // Allocate next fake address
  Ipv4Address fakeAddr (fakeCounter);
  fakeCounter++;
  if (fakeCounter > m_fakePoolEnd) {
    fakeCounter = m_fakePoolStart; // Wrap around
  }
  
  // Use very short lease for fake addresses
  m_leases[chaddr] = std::make_pair (fakeAddr, m_starvationLease);
  
  NS_LOG_INFO ("Allocated fake address " << fakeAddr << " to " << chaddr);
  return fakeAddr;
}

void
RogueDhcpServer::AddLegitimateClient (const Mac48Address &chaddr)
{
  m_legitimateClients.insert (chaddr);
  NS_LOG_INFO ("Added legitimate client: " << chaddr);
}

void
RogueDhcpServer::SendSpoofedOffer (DhcpHeader &discoverHdr, const Address &from)
{
  DhcpHeader offer;
  Ptr<Packet> packet = Create<Packet> ();
  
  offer.ResetOpt ();
  offer.SetType (DhcpHeader::DHCPOFFER);
  offer.SetTran (discoverHdr.GetTran ());
  offer.SetChaddr (discoverHdr.GetChaddr ());
  
  // Allocate a fake IP from our pool
  Mac48Address chaddr = Mac48Address::ConvertFrom (discoverHdr.GetChaddr ());
  Ipv4Address offeredIp = AllocateAddress (chaddr, true);
  offer.SetYiaddr (offeredIp);
  
  // Get our own address to use as gateway
  Ipv4Address ourAddr = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  
  // Set fake DHCP options using the correct API
  offer.SetMask (m_netmask.Get ());
  offer.SetRouter (ourAddr);
  offer.SetLease (m_defaultLease.GetSeconds ());
  offer.SetRenew (m_defaultLease.GetSeconds () / 2); // T1 = 50% of lease time
  offer.SetRebind (m_defaultLease.GetSeconds () * 7 / 8); // T2 = 87.5% of lease time
  offer.SetDhcps (ourAddr);
  
  packet->AddHeader (offer);
  m_socket->SendTo (packet, 0, from);
  
  NS_LOG_INFO ("Sent spoofed OFFER for " << offeredIp);
}

void
RogueDhcpServer::SendSpoofedAck (DhcpHeader &requestHdr, const Address &from)
{
  DhcpHeader ack;
  Ptr<Packet> packet = Create<Packet> ();
  
  ack.ResetOpt ();
  ack.SetType (DhcpHeader::DHCPACK);
  ack.SetTran (requestHdr.GetTran ());
  ack.SetChaddr (requestHdr.GetChaddr ());
  
  // Use the requested IP or allocate a new one
  Mac48Address chaddr = Mac48Address::ConvertFrom (requestHdr.GetChaddr ());
  Ipv4Address ackIp = requestHdr.GetYiaddr ();
  if (ackIp == Ipv4Address::GetAny ()) {
    ackIp = AllocateAddress (chaddr);
  }
  ack.SetYiaddr (ackIp);
  
  // Get our own address to use as gateway
  Ipv4Address ourAddr = GetNode ()->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  
  // Set fake DHCP options using the correct API
  ack.SetMask (m_netmask.Get ());
  ack.SetRouter (ourAddr);
  ack.SetLease (m_defaultLease.GetSeconds ());
  ack.SetRenew (m_defaultLease.GetSeconds () / 2); // T1 = 50% of lease time
  ack.SetRebind (m_defaultLease.GetSeconds () * 7 / 8); // T2 = 87.5% of lease time
  ack.SetDhcps (ourAddr);
  
  packet->AddHeader (ack);
  m_socket->SendTo (packet, 0, from);
  
  NS_LOG_INFO ("Sent spoofed ACK for " << ackIp);
}

} // namespace ns3