set_global_assignment -entity "enet_gtx_clk_ddio" -library "lib_enet_gtx_clk_ddio" -name IP_TOOL_NAME "altera_gpio_lite"
set_global_assignment -entity "enet_gtx_clk_ddio" -library "lib_enet_gtx_clk_ddio" -name IP_TOOL_VERSION "20.1"
set_global_assignment -entity "enet_gtx_clk_ddio" -library "lib_enet_gtx_clk_ddio" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_enet_gtx_clk_ddio" -name SPD_FILE [file join $::quartus(sip_path) "enet_gtx_clk_ddio.spd"]

set_global_assignment -library "lib_enet_gtx_clk_ddio" -name MISC_FILE [file join $::quartus(sip_path) "enet_gtx_clk_ddio_sim/enet_gtx_clk_ddio.v"]
set_global_assignment -library "lib_enet_gtx_clk_ddio" -name MISC_FILE [file join $::quartus(sip_path) "enet_gtx_clk_ddio_sim/altera_gpio_lite/altera_gpio_lite.sv"]
