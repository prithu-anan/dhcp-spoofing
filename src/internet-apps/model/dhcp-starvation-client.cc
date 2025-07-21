#include "dhcp-starvation-client.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/dhcp-header.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DhcpStarvationClient");
NS_OBJECT_ENSURE_REGISTERED (DhcpStarvationClient);

TypeId
DhcpStarvationClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DhcpStarvationClient")
    .SetParent<Application> ()
    .AddConstructor<DhcpStarvationClient> ()
    .AddAttribute ("Interval", "Time between each DISCOVER flood",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor (&DhcpStarvationClient::m_interval),
                   MakeTimeChecker ());
  return tid;
}

DhcpStarvationClient::DhcpStarvationClient ()
  : m_socket (0), m_rand (CreateObject<UniformRandomVariable> ()), m_running (false)
{
}

DhcpStarvationClient::~DhcpStarvationClient ()
{
}

void
DhcpStarvationClient::StartApplication (void)
{
  m_running = true;
  if (!m_socket) {
    m_socket = Socket::CreateSocket (GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
    m_socket->SetAllowBroadcast (true);
    m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), 68));
  }
  SendDiscover ();
}

void
DhcpStarvationClient::StopApplication (void)
{
  m_running = false;
  if (m_sendEvent.IsPending ()) {
    Simulator::Cancel (m_sendEvent);
  }
  if (m_socket) {
    m_socket->Close ();
  }
}

void
DhcpStarvationClient::ScheduleDiscover (void)
{
  if (m_running) {
    m_sendEvent = Simulator::Schedule (m_interval, &DhcpStarvationClient::SendDiscover, this);
  }
}

void
DhcpStarvationClient::SendDiscover (void)
{
  DhcpHeader hdr;
  Ptr<Packet> packet = Create<Packet> ();
  hdr.ResetOpt ();
  hdr.SetType (DhcpHeader::DHCPDISCOVER);
  hdr.SetTran (m_rand->GetValue ());
  // random MAC
  uint8_t mac[6]; for (int i = 0; i < 6; ++i) { mac[i] = m_rand->GetInteger (0,255); }
  hdr.SetChaddr (Address (1, mac, 6));
  packet->AddHeader (hdr);
  m_socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address ("255.255.255.255"), 67));
  NS_LOG_INFO ("Sent starvation DISCOVER");
  ScheduleDiscover ();
}

} // namespace ns3