#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/csma-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DhcpSpoofSimpleExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd; cmd.Parse (argc, argv);

  // Create a simple network with 3 nodes
  NodeContainer nodes; 
  nodes.Create (3);
  auto client = nodes.Get (0);
  auto legit  = nodes.Get (1);
  auto rogue  = nodes.Get (2);

  // Install internet stack on all nodes
  InternetStackHelper internet; 
  internet.Install (nodes);
  
  // Create CSMA network
  CsmaHelper csma; 
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay",    TimeValue (MilliSeconds (1)));
  auto devs = csma.Install (nodes);
  
  // Assign IP addresses
  Ipv4AddressHelper addr;
  addr.SetBase ("10.0.0.0", "255.255.255.0");
  addr.Assign (devs);

  // Legitimate DHCP server
  DhcpHelper dhcp;
  dhcp.SetServerAttribute ("StartTime", TimeValue (Seconds (0.1)));
  // Install DHCP server with basic parameters
  dhcp.InstallDhcpServer (devs.Get (1), // legit node's device
                         Ipv4Address ("10.0.0.1"), // server address
                         Ipv4Address ("10.0.0.0"), // pool network
                         Ipv4Mask ("255.255.255.0"), // pool mask
                         Ipv4Address ("10.0.0.10"), // min address
                         Ipv4Address ("10.0.0.50"), // max address
                         Ipv4Address ("10.0.0.1")); // gateway

  // Install DHCP clients on the other nodes to generate traffic
  NetDeviceContainer clientDevices;
  clientDevices.Add (devs.Get (0)); // client node
  clientDevices.Add (devs.Get (2)); // rogue node (will also act as client)
  
  ApplicationContainer dhcpClients = dhcp.InstallDhcpClient (clientDevices);
  dhcpClients.Start (Seconds (0.5)); // Start clients after server is ready

  // Add a simple echo server to test connectivity after DHCP
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (legit);
  serverApps.Start (Seconds (2.0)); // Start server after DHCP is complete

  // Add echo client to test connectivity
  UdpEchoClientHelper echoClient (Ipv4Address ("10.0.0.1"), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (client);
  clientApps.Start (Seconds (3.0)); // Start client after DHCP and server are ready

  // Enable comprehensive logging for DHCP components
  LogComponentEnable ("DhcpSpoofSimpleExample", LOG_LEVEL_INFO);
  LogComponentEnable ("DhcpServer",            LOG_LEVEL_INFO);
  LogComponentEnable ("DhcpHelper",            LOG_LEVEL_INFO);
  LogComponentEnable ("DhcpClient",            LOG_LEVEL_INFO);
  LogComponentEnable ("DhcpHeader",            LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Add some console output
  std::cout << "Starting DHCP Spoof Simple Example..." << std::endl;
  std::cout << "Network setup complete with 3 nodes" << std::endl;
  std::cout << "DHCP server configured on node 1" << std::endl;
  std::cout << "DHCP clients will start at 0.5 seconds" << std::endl;
  std::cout << "Simulation will run for 5 seconds to see complete DHCP handshake" << std::endl;

  // Run simulation for a longer duration to see complete DHCP handshake
  Simulator::Stop (Seconds (5.0));
  Simulator::Run ();
  
  std::cout << "Simulation completed successfully!" << std::endl;
  std::cout << "DHCP clients should have received IP addresses from the server" << std::endl;
  Simulator::Destroy ();
  return 0;
} 