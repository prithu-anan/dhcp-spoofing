#ifndef ROGUE_DHCP_SERVER_H
#define ROGUE_DHCP_SERVER_H

#include "ns3/dhcp-server.h"
#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"
#include "ns3/socket.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include <map>
#include <vector>
#include <set>

namespace ns3 {

class RogueDhcpServer : public DhcpServer
{
public:
  static TypeId GetTypeId (void);
  RogueDhcpServer ();
  virtual ~RogueDhcpServer ();

protected:
  virtual void StartApplication (void) override;
  virtual void StopApplication (void) override;

private:
  // Lease bookkeeping: client MAC -> (IP, remaining lease time)
  std::map<Mac48Address, std::pair<Ipv4Address, Time>> m_leases;
  std::vector<Ipv4Address> m_available;
  Time m_defaultLease;
  Ipv4Mask m_netmask;
  EventId m_timerEvent;
  Ptr<Socket> m_socket;  // Our own socket for intercepting packets
  
  // Anti-starvation features
  uint32_t m_poolStart;           // Starting address of pool
  uint32_t m_poolEnd;             // Ending address of pool
  uint32_t m_fakePoolStart;       // Starting address for fake pool
  uint32_t m_fakePoolEnd;         // Ending address for fake pool
  bool m_useFakeAddresses;        // Whether to use fake addresses when real pool is exhausted
  bool m_dynamicExpansion;        // Whether to dynamically expand the pool
  uint32_t m_expansionSize;       // How many addresses to add when expanding
  Time m_starvationLease;         // Very short lease for starvation attacks
  std::set<Mac48Address> m_legitimateClients; // Track legitimate clients
  Time m_reservationLease;
  
  void NetHandler (Ptr<Socket> socket);
  void TimerHandler (void);
  void SendSpoofedOffer (DhcpHeader &discoverHdr, const Address &from);
  void SendSpoofedAck   (DhcpHeader &requestHdr,  const Address &from);
  void CleanupExpiredLeases (void);
  Ipv4Address AllocateAddress (const Mac48Address &chaddr, bool isDiscover);
  
  // Anti-starvation methods
  bool IsStarvationAttack (const Mac48Address &chaddr);
  void ExpandPool (void);
  Ipv4Address AllocateFakeAddress (const Mac48Address &chaddr);
  void AddLegitimateClient (const Mac48Address &chaddr);
};

} // namespace ns3

#endif // ROGUE_DHCP_SERVER_H