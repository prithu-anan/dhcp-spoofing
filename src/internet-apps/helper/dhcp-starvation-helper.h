#ifndef DHCP_STARVATION_HELPER_H
#define DHCP_STARVATION_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

class DhcpStarvationHelper
{
public:
  DhcpStarvationHelper ();
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (NodeContainer c) const;
private:
  ObjectFactory m_factory;
};

} // namespace ns3

#endif // DHCP_STARVATION_HELPER_H