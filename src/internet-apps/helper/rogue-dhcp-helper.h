#ifndef ROGUE_DHCP_HELPER_H
#define ROGUE_DHCP_HELPER_H

#include "ns3/application-container.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"

namespace ns3 {

class RogueDhcpHelper
{
public:
  RogueDhcpHelper ();
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (NodeContainer c) const;

private:
  ObjectFactory m_factory;
};

} // namespace ns3

#endif // ROGUE_DHCP_HELPER_H