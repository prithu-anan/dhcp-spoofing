build_lib_example(
  NAME dhcp-example
  SOURCE_FILES dhcp-example.cc
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libinternet-apps}
    ${libcsma}
    ${libpoint-to-point}
    ${libapplications}
)

build_lib_example(
  NAME dhcp6-example
  SOURCE_FILES dhcp6-example.cc
  LIBRARIES_TO_LINK
    ${libapplications}
    ${libcsma}
    ${libinternet}
    ${libinternet-apps}
    ${libpoint-to-point}
    ${libwifi}
)

build_lib_example(
  NAME traceroute-example
  SOURCE_FILES traceroute-example.cc
  LIBRARIES_TO_LINK
    ${libaodv}
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libpoint-to-point}
    ${libwifi}
    ${libinternet-apps}
)

build_lib_example(
  NAME ping-example
  SOURCE_FILES ping-example.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libinternet}
    ${libinternet-apps}
    ${libnix-vector-routing}
    ${libpoint-to-point}
)

build_lib_example(
  NAME        dhcp-spoof-simple-example
  SOURCE_FILES dhcp-spoof-simple-example.cc
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libinternet-apps}
    ${libcsma}
    ${libpoint-to-point}
    ${libapplications}
)

build_lib_example(
  NAME        dhcp-spoof-enhanced-example
  SOURCE_FILES dhcp-spoof-enhanced-example.cc
  LIBRARIES_TO_LINK
    ${libinternet}
    ${libinternet-apps}
    ${libcsma}
    ${libpoint-to-point}
    ${libapplications}
)
