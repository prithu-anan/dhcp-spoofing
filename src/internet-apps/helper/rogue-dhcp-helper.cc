#include "rogue-dhcp-helper.h"
#include "ns3/rogue-dhcp-server.h"

namespace ns3 {

RogueDhcpHelper::RogueDhcpHelper ()
{
  m_factory.SetTypeId ("ns3::RogueDhcpServer");
}

void
RogueDhcpHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
RogueDhcpHelper::Install (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);
  return ApplicationContainer (app);
}

ApplicationContainer
RogueDhcpHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (uint32_t i = 0; i < c.GetN (); ++i) {
    apps.Add (Install (c.Get (i)));
  }
  return apps;
}

} // namespace ns3
