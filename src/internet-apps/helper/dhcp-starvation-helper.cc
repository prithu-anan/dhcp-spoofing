#include "dhcp-starvation-helper.h"
#include "ns3/dhcp-starvation-client.h"

namespace ns3 {

DhcpStarvationHelper::DhcpStarvationHelper ()
{
  m_factory.SetTypeId ("ns3::DhcpStarvationClient");
}

void
DhcpStarvationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DhcpStarvationHelper::Install (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);
  return ApplicationContainer (app);
}

ApplicationContainer
DhcpStarvationHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (uint32_t i = 0; i < c.GetN (); ++i) {
    apps.Add (Install (c.Get (i)));
  }
  return apps;
}

} // namespace ns3