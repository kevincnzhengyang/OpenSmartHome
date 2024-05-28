# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_wifi wifi_provisioning nvs_flash esp_netif driver cbor mbedtls)

set(COMPONENT_SRCS "src/osh_node.c" "src/osh_node_fsm.c" "src/osh_node_status.c"
    "src/osh_node_ota.c" "src/osh_node_wifi.c" "src/osh_node_proto.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

register_component()