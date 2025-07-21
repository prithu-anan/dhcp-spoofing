#ifndef DHCP_STARVATION_CLIENT_H
#define DHCP_STARVATION_CLIENT_H

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"

namespace ns3 {

class DhcpStarvationClient : public Application
{
public:
  static TypeId GetTypeId (void);
  DhcpStarvationClient ();
  virtual ~DhcpStarvationClient ();

protected:
  virtual void StartApplication (void) override;
  virtual void StopApplication  (void) override;

private:
  void ScheduleDiscover (void);
  void SendDiscover   (void);

  Ptr<Socket> m_socket;
  Ptr<UniformRandomVariable> m_rand;
  EventId m_sendEvent;
  Time m_interval;
  bool m_running;
};

} // namespace ns3

#endif // DHCP_STARVATION_CLIENT_H