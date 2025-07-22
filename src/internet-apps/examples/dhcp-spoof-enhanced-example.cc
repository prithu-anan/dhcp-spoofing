#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/csma-module.h"
#include "ns3/rogue-dhcp-helper.h"
#include "ns3/dhcp-starvation-helper.h"
#include <fstream>
#include <sstream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DhcpSpoofEnhancedExample");

// Global variables to track lease information
Ipv4Address g_clientLeasedAddress = Ipv4Address ("0.0.0.0");
Ipv4Address g_dhcpServerAddress = Ipv4Address ("0.0.0.0");
bool g_leaseObtained = false;

// Function to append results to CSV file
void AppendToCSV(uint32_t nClients, uint32_t nAddr, double starvationStopTime, 
                 double clientStartInterval, uint32_t starvationInterval, 
                 int rogueCount, int legitimateCount, int noAddressCount)
{
  std::ofstream csvFile;
  csvFile.open("dhcp-spoof-results.csv", std::ios::app); // Open in append mode
  
  if (!csvFile.is_open())
  {
    NS_LOG_ERROR("Failed to open CSV file for writing");
    return;
  }
  
  // Calculate percentage of rogue addresses
  double roguePercentage = 0.0;
  if (nClients > 0)
  {
    roguePercentage = (static_cast<double>(rogueCount) / nClients) * 100.0;
  }
  
  // Write CSV record
  csvFile << nClients << ","
          << nAddr << ","
          << starvationStopTime << ","
          << clientStartInterval << ","
          << starvationInterval << ","
          << rogueCount << ","
          << legitimateCount << ","
          << noAddressCount << ","
          << roguePercentage << "\n";
  
  csvFile.close();
  NS_LOG_INFO("Results appended to dhcp-spoof-results.csv");
}

// Function to create CSV header if file doesn't exist
void CreateCSVHeader()
{
  std::ifstream checkFile("dhcp-spoof-results.csv");
  if (!checkFile.good())
  {
    std::ofstream csvFile("dhcp-spoof-results.csv");
    csvFile << "nClients,nAddr,starvationStopTime,clientStartInterval,starvationInterval,"
            << "rogueCount,legitimateCount,noAddressCount,roguePercentage\n";
    csvFile.close();
    NS_LOG_INFO("Created new CSV file with header");
  }
  else
  {
    checkFile.close();
  }
}

// Callback function to track lease assignments
void LeaseObtained (std::string context, const Ipv4Address& leasedAddress)
{
  NS_LOG_INFO ("[" << Simulator::Now().As(Time::S) << "] " 
                   << context << " obtained lease: " << leasedAddress);
  g_clientLeasedAddress = leasedAddress;
  g_leaseObtained = true;
}

void LeaseExpired (std::string context, const Ipv4Address& expiredAddress)
{
  NS_LOG_INFO ("[" << Simulator::Now().As(Time::S) << "] " 
                   << context << " lease expired: " << expiredAddress);
}

// Function to check client's final address assignment
void CheckClientAddress (Ptr<Node> clientNode, uint32_t nAddr)
{
  Ptr<Ipv4> ipv4 = clientNode->GetObject<Ipv4>();
  int32_t ifIndex = ipv4->GetInterfaceForDevice (clientNode->GetDevice (1));
  
  // Calculate the legitimate address range based on nAddr
  uint32_t legitMinAddrValue = (10 << 24) + (0 << 16) + (10 << 8) + 10; // 10.0.10.10
  uint32_t legitMaxAddrValue = legitMinAddrValue + nAddr - 1; // Calculated max address
  Ipv4Address legitMinAddr = Ipv4Address(legitMinAddrValue);
  Ipv4Address legitMaxAddr = Ipv4Address(legitMaxAddrValue);
  
  NS_LOG_INFO ("\n=== CLIENT ADDRESS ANALYSIS ===");
  NS_LOG_INFO ("Client node " << clientNode->GetId() << " addresses:");
  
  for (uint32_t i = 0; i < ipv4->GetNAddresses (ifIndex); i++)
  {
    Ipv4InterfaceAddress addr = ipv4->GetAddress (ifIndex, i);
    NS_LOG_INFO ("  Address " << i << ": " << addr.GetLocal() 
                              << "/" << addr.GetMask());
  }
  
  // Check if client has a non-zero IP (indicating successful DHCP assignment)
  if (ipv4->GetNAddresses (ifIndex) > 1) // More than just 0.0.0.0
  {
    Ipv4Address assignedAddr = ipv4->GetAddress (ifIndex, 1).GetLocal();
    if (assignedAddr != Ipv4Address ("0.0.0.0"))
    {
      NS_LOG_INFO ("✓ SUCCESS: Client assigned IP " << assignedAddr);
      
      // Check if it's from rogue server's range (10.0.0.100-200)
      if (assignedAddr.Get() >= Ipv4Address("10.0.0.100").Get() && 
          assignedAddr.Get() <= Ipv4Address("10.0.0.254").Get())
      {
        NS_LOG_INFO ("⚠ ROGUE SERVER SUCCESS: Client got IP from rogue server range!");
        NS_LOG_INFO ("   Rogue server IP range: 10.0.0.100 - 10.0.0.254");
      }
      else if (assignedAddr.Get() >= legitMinAddr.Get() && 
               assignedAddr.Get() <= legitMaxAddr.Get())
      {
        NS_LOG_INFO ("✓ LEGITIMATE SERVER: Client got IP from legitimate server range");
        NS_LOG_INFO ("   Legitimate server IP range: " << legitMinAddr << " - " << legitMaxAddr);
      }
      else
      {
        NS_LOG_INFO ("? UNKNOWN SOURCE: Client got IP " << assignedAddr << " from unknown range");
      }
    }
    else
    {
      NS_LOG_INFO ("✗ FAILURE: Client has no valid IP assignment");
    }
  }
  else
  {
    NS_LOG_INFO ("✗ FAILURE: Client has no IP addresses");
  }
  
  // Summary of attack success
  if (g_leaseObtained)
  {
    NS_LOG_INFO ("\n=== ATTACK SUMMARY ===");
    NS_LOG_INFO ("Client leased address: " << g_clientLeasedAddress);
    NS_LOG_INFO ("DHCP server address: " << g_dhcpServerAddress);
    
    if (g_clientLeasedAddress.Get() >= Ipv4Address("10.0.0.100").Get() && 
        g_clientLeasedAddress.Get() <= Ipv4Address("10.0.0.255").Get())
    {
      NS_LOG_INFO ("🎯 ROGUE DHCP ATTACK SUCCESSFUL!");
      NS_LOG_INFO ("   Client was assigned an IP by the rogue server");
    }
    else
    {
      NS_LOG_INFO ("✅ LEGITIMATE DHCP SUCCESS");
      NS_LOG_INFO ("   Client was assigned an IP by the legitimate server");
    }
  }
  else
  {
    NS_LOG_INFO ("\n=== ATTACK SUMMARY ===");
    NS_LOG_INFO ("❌ NO DHCP LEASE OBTAINED");
    NS_LOG_INFO ("   Client failed to obtain any IP address");
  }
}

int
main (int argc, char *argv[])
{
  // Create CSV header if file doesn't exist
  CreateCSVHeader();
  
  uint32_t nClients = 5; // Default number of clients
  uint32_t nAddr = 6; // Default number of legitimate addresses (10.0.0.10-15 = 6 addresses)
  double starvationStopTime = 2.0; // Default starvation stop time
  double clientStartInterval = 0.8; // Default interval between client starts
  uint32_t starvationInterval = 10; // Default starvation interval in milliseconds
  bool logEnabled = false;
  bool pcapEnabled = true; // Default to enable PCAP generation
  
  CommandLine cmd;
  cmd.AddValue ("nClients", "Number of clients to simulate", nClients);
  cmd.AddValue ("nAddr", "Number of legitimate DHCP server addresses", nAddr);
  cmd.AddValue ("starvStopTime", "Time to stop starvation attack (seconds)", starvationStopTime);
  cmd.AddValue ("clientStartInterval", "Interval between client start times (seconds)", clientStartInterval);
  cmd.AddValue ("starvInterval", "Starvation attack interval (milliseconds)", starvationInterval);
  cmd.AddValue ("logEnabled", "Enable logging to file", logEnabled);
  cmd.AddValue ("pcapEnabled", "Enable PCAP file generation", pcapEnabled);
  cmd.Parse (argc, argv);

  // Calculate the max address based on number of addresses
  // Start from 10.0.10.10, so max = 10.0.10.10 + nAddr - 1
  uint32_t maxAddrValue = (10 << 24) + (0 << 16) + (10 << 8) + 10 + nAddr - 1;
  std::string legitMaxAddr = std::to_string((maxAddrValue >> 24) & 0xFF) + "." +
                            std::to_string((maxAddrValue >> 16) & 0xFF) + "." +
                            std::to_string((maxAddrValue >> 8) & 0xFF) + "." +
                            std::to_string(maxAddrValue & 0xFF);

  NodeContainer nodes; 
  nodes.Create (2 + nClients + 1); // 1 starver + nClients + 1 legit server + 1 rogue server
  
  auto starver = nodes.Get (0);
  auto legit  = nodes.Get (nClients + 1);
  auto rogue  = nodes.Get (nClients + 2);

  InternetStackHelper internet; internet.Install (nodes);
  CsmaHelper csma; csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
                csma.SetChannelAttribute ("Delay",    TimeValue (MilliSeconds (1)));
  auto devs = csma.Install (nodes);
  
  // Enable PCAP tracing on the CSMA channel if enabled
  if (pcapEnabled)
    {
      // Remove existing traces directory and create a fresh one
      std::string tracesDir = "traces";
      std::string rmCmd = "rm -rf " + tracesDir;
      std::string mkdirCmd = "mkdir -p " + tracesDir;
      system(rmCmd.c_str());
      system(mkdirCmd.c_str());
      
      // Enable PCAP with traces directory path
      csma.EnablePcap (tracesDir + "/dhcp-spoof-enhanced", devs);
      NS_LOG_INFO ("PCAP tracing enabled - fresh traces directory created: traces/dhcp-spoof-enhanced-*.pcap");
    }
  Ipv4AddressHelper addr;
  addr.SetBase ("10.0.0.0", "255.255.255.0");
  addr.Assign (devs);

  // Flood the legit server to exhaust its pool
  DhcpStarvationHelper starv;
  starv.SetAttribute ("Interval", TimeValue (MilliSeconds (starvationInterval))); // Configurable starvation speed
  ApplicationContainer starvApps = starv.Install (starver);
  starvApps.Start (Seconds (0.0));
  starvApps.Stop (Seconds (starvationStopTime)); // Configurable starvation stop time

  // Legitimate DHCP server (starts first)
  DhcpHelper dhcp;
  dhcp.SetServerAttribute ("StartTime", TimeValue (Seconds (0.1))); // Start early
  // Install DHCP server with configurable pool size
  dhcp.InstallDhcpServer (devs.Get (nClients + 1), // legit node's device
                         Ipv4Address ("10.0.0.1"), // server address
                         Ipv4Address ("10.0.0.0"), // pool network
                         Ipv4Mask ("255.255.255.0"), // pool mask
                         Ipv4Address ("10.0.10.10"), // min address
                         Ipv4Address (legitMaxAddr.c_str ()), // max address - calculated from nAddr
                         Ipv4Address ("10.0.0.1")); // gateway

  // Rogue DHCP server (starts after starvation begins)
  RogueDhcpHelper rogueHelper;
  rogueHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.5))); // Start during starvation
  rogueHelper.SetAttribute ("UseFakeAddresses", BooleanValue (true)); // Use fake addresses when pool is exhausted
  rogueHelper.SetAttribute ("DynamicExpansion", BooleanValue (true)); // Dynamically expand pool
  rogueHelper.SetAttribute ("ExpansionSize", UintegerValue (50)); // 50 addresses to add when expanding
  rogueHelper.SetAttribute ("StarvationLease", TimeValue (Seconds (30))); // Short lease for starvation attacks
  rogueHelper.Install (rogue);

  // Create nClients legitimate clients (start at different times to see different outcomes)
  std::vector<DhcpHelper> clientHelpers(nClients);
  std::vector<ApplicationContainer> clientApps(nClients);
  std::vector<Ptr<Node>> clients(nClients);
  
  for (uint32_t i = 0; i < nClients; i++)
    {
      clients[i] = nodes.Get (i + 1); // Client nodes start from index 1
      
      // Distribute client start times across the simulation with configurable interval
      double startTime = 0.3 + (i * clientStartInterval); // Configurable client start interval
      clientHelpers[i].SetClientAttribute ("StartTime", TimeValue (Seconds (startTime)));
      clientApps[i] = clientHelpers[i].InstallDhcpClient (devs.Get (i + 1));
      
      // Connect tracing callbacks to monitor lease assignments for each client
      std::string clientName = "Client" + std::to_string(i + 1);
      clientApps[i].Get (0)->TraceConnect ("NewLease", clientName, MakeCallback (&LeaseObtained));
    }

  if (logEnabled)
  {
    LogComponentEnable ("DhcpSpoofEnhancedExample", LOG_LEVEL_INFO);
    LogComponentEnable ("DhcpStarvationClient",   LOG_LEVEL_INFO);
    LogComponentEnable ("RogueDhcpServer",       LOG_LEVEL_INFO);
    LogComponentEnable ("DhcpClient",            LOG_LEVEL_INFO);
    LogComponentEnable ("DhcpServer",            LOG_LEVEL_INFO);
  }

  Simulator::Stop (Seconds (10.0)); // Extended simulation time
  Simulator::Run ();
  
  // Post-simulation analysis for all clients
  NS_LOG_INFO ("\n=== MULTI-CLIENT ADDRESS ANALYSIS ===");
  
  // Calculate the legitimate address range based on nAddr
  uint32_t legitMinAddrValue = (10 << 24) + (0 << 16) + (10 << 8) + 10; // 10.0.10.10
  uint32_t legitMaxAddrValue = legitMinAddrValue + nAddr - 1; // Calculated max address
  Ipv4Address legitMinAddr = Ipv4Address(legitMinAddrValue);
  Ipv4Address legitMaxAddrForAnalysis = Ipv4Address(legitMaxAddrValue);
  
  NS_LOG_INFO ("Legitimate server pool: " << legitMinAddr << " - " << legitMaxAddrForAnalysis 
                                          << " (" << nAddr << " addresses)");
  
  std::vector<std::string> clientNames(nClients);
  for (uint32_t i = 0; i < nClients; i++)
    {
      clientNames[i] = "Client" + std::to_string(i + 1);
    }
  
  int legitimateCount = 0;
  int rogueCount = 0;
  int noAddressCount = 0;
  
  for (uint32_t i = 0; i < nClients; i++)
    {
      Ptr<Node> client = clients[i];
      std::string clientName = clientNames[i];
      
      NS_LOG_INFO ("\n--- " << clientName << " Analysis ---");
      Ptr<Ipv4> ipv4 = client->GetObject<Ipv4> ();
      NS_LOG_INFO (clientName << " network interfaces:");
      
      bool hasRogueAddress = false;
      bool hasLegitimateAddress = false;
      Ipv4Address rogueAddress = Ipv4Address ("0.0.0.0");
      Ipv4Address legitimateAddress = Ipv4Address ("0.0.0.0");
      
      for (uint32_t j = 0; j < ipv4->GetNInterfaces (); j++)
        {
          std::string interfaceType = "";
          if (j == 0)
            {
              interfaceType = " (loopback)";
            }
          else if (j == 1)
            {
              interfaceType = " (CSMA network)";
            }
          else
            {
              interfaceType = " (additional)";
            }
          
          NS_LOG_INFO ("  Interface " << j << interfaceType << ":");
          for (uint32_t k = 0; k < ipv4->GetNAddresses (j); k++)
            {
              Ipv4InterfaceAddress addr = ipv4->GetAddress (j, k);
              std::string addrType = "";
              if (addr.GetLocal() == Ipv4Address("127.0.0.1"))
                {
                  addrType = " (loopback)";
                }
              else if (addr.GetLocal() == Ipv4Address("0.0.0.0"))
                {
                  addrType = " (unassigned)";
                }
              else
                {
                  addrType = " (assigned)";
                }
              NS_LOG_INFO ("    Address " << k << ": " << addr.GetLocal () << "/" << addr.GetMask () << addrType);
              
              // Check if this is a rogue address (10.0.0.201-254 range)
              if (addr.GetLocal ().Get () >= Ipv4Address ("10.0.0.100").Get () && 
                  addr.GetLocal ().Get () <= Ipv4Address ("10.0.0.254").Get ())
                {
                  hasRogueAddress = true;
                  rogueAddress = addr.GetLocal ();
                }
              // Check if this is a legitimate address (dynamic range based on nAddr)
              else if (addr.GetLocal ().Get () >= legitMinAddr.Get () && 
                       addr.GetLocal ().Get () <= legitMaxAddrForAnalysis.Get ())
                {
                  hasLegitimateAddress = true;
                  legitimateAddress = addr.GetLocal ();
                }
            }
        }
      
      if (hasRogueAddress)
        {
          NS_LOG_INFO ("🎯 ROGUE: " << clientName << " got rogue IP " << rogueAddress);
          rogueCount++;
        }
      else if (hasLegitimateAddress)
        {
          NS_LOG_INFO ("✅ LEGITIMATE: " << clientName << " got legitimate IP " << legitimateAddress);
          legitimateCount++;
        }
      else
        {
          NS_LOG_INFO ("❌ NO ADDRESS: " << clientName << " has no valid IP address");
          noAddressCount++;
        }
    }
  
  // Summary statistics
  NS_LOG_INFO ("\n=== ATTACK SUMMARY STATISTICS ===");
  NS_LOG_INFO ("Total clients: " << nClients);
  NS_LOG_INFO ("✅ Legitimate addresses: " << legitimateCount << "/" << nClients);
  NS_LOG_INFO ("🎯 Rogue addresses: " << rogueCount << "/" << nClients);
  NS_LOG_INFO ("❌ No addresses: " << noAddressCount << "/" << nClients);
  
  if (rogueCount > legitimateCount)
    {
      NS_LOG_INFO ("🎯 ROGUE DHCP ATTACK SUCCESSFUL!");
      NS_LOG_INFO ("   More clients got rogue addresses than legitimate addresses");
    }
  else if (legitimateCount > rogueCount)
    {
      NS_LOG_INFO ("✅ LEGITIMATE DHCP DOMINANT");
      NS_LOG_INFO ("   More clients got legitimate addresses than rogue addresses");
    }
  else
    {
      NS_LOG_INFO ("⚖️ MIXED RESULTS");
      NS_LOG_INFO ("   Equal number of clients got legitimate and rogue addresses");
    }
  
  Simulator::Destroy ();

  // Append results to CSV file
  AppendToCSV(nClients, nAddr, starvationStopTime, clientStartInterval, starvationInterval, 
              rogueCount, legitimateCount, noAddressCount);

  // Summary of output files
  if (pcapEnabled)
    {
      NS_LOG_INFO ("\n=== OUTPUT FILES ===");
      NS_LOG_INFO ("PCAP files generated:");
      NS_LOG_INFO ("  - traces/dhcp-spoof-enhanced-*.pcap (network traffic capture)");
      NS_LOG_INFO ("CSV file:");
      NS_LOG_INFO ("  - dhcp-spoof-results.csv (attack statistics)");
      NS_LOG_INFO ("");
      NS_LOG_INFO ("To analyze PCAP files, use tools like:");
      NS_LOG_INFO ("  - Wireshark: wireshark traces/dhcp-spoof-enhanced-*.pcap");
      NS_LOG_INFO ("  - tcpdump: tcpdump -r traces/dhcp-spoof-enhanced-*.pcap");
      NS_LOG_INFO ("  - tshark: tshark -r traces/dhcp-spoof-enhanced-*.pcap -Y 'dhcp'");
    }

  return 0;
}