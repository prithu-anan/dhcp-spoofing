./ns3 configure --enable-examples --enable-tests

./ns3 build

./ns3 run "dhcp-spoof-enhanced-example --nClients=50 --nAddr=200 --starvStopTime=6.0 --clientStartInterval=0.1 --starvInterval=2 --logEnabled=true"
