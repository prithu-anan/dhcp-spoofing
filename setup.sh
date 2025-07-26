./ns3 configure --enable-examples --enable-tests

./ns3 build

./ns3 run "dhcp-spoof-enhanced-example --nClients=20 --nAddr=50 --starvStopTime=8.0 --clientStartInterval=0.2 --starvInterval=5 --logEnabled=true"

