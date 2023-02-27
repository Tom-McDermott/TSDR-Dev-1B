#**************************************************************
# Create Clock
#**************************************************************

derive_pll_clocks

# JTAG Signal Constraints constrain the TCK port, assuming a 10MHz JTAG clock and 3ns delays
create_clock -name {altera_reserved_tck} -period 41.667 [get_ports { altera_reserved_tck }]
set_input_delay -clock altera_reserved_tck -clock_fall -max 5 [get_ports altera_reserved_tdi]
set_input_delay -clock altera_reserved_tck -clock_fall -max 5 [get_ports altera_reserved_tms]
set_output_delay -clock altera_reserved_tck 5 [get_ports altera_reserved_tdo]

create_clock -name {clk_50_max10} -period 20.000 {clk_50_max10}

################################ SDC for RGMII interface #################################################
#set enet_pll_125 { q_sys_inst|enet_pll|sd1|pll7|clk[0] };
#set enet_pll_25  { q_sys_inst|enet_pll|sd1|pll7|clk[1] };
#set enet_pll_2p5 { q_sys_inst|enet_pll|sd1|pll7|clk[2] };

set enet_pll_125 { q_sys_inst|enet_pll|sd1|pll7|clk[3] };
set enet_pll_25  { q_sys_inst|enet_pll|sd1|pll7|clk[4] };
set enet_pll_2p5 { q_sys_inst|enet_pll|sd1|pll7|clk[2] };

#call sdc files for rgmii interface
source rgmii_sdc/rgmii_clocks.sdc
source rgmii_sdc/rgmii_input.sdc
source rgmii_sdc/rgmii_output.sdc

#
# Create false paths between clock domains that are not fully constrained by the IP that makes these paths.
# Ideally we should not have to do this, however, these paths must be cut to meet timing and in most cases
# the IP should be accounting for synchronization between these paths.
# The risk of a global cut like these is that if the IP is not synchronizing properly between the two domains
# then you have significant problems with the design.  Ideally the IP should synchronize it's clock crossing
# paths and create constraints to cut those paths so we don't have to perform a global cut like this.
#

set_clock_groups \
    -exclusive \
    -group [get_clocks {tx_clk_125 tx_clk_25 tx_clk_2p5}] \
    -group [get_clocks enet_rx_clk] \
    -group [get_clocks clk_50_max10]

##
## These are the constraints out of the TSE MAC SDC file that are relevant to us.
## The TSE MAC SDC file should be commented out by this script.
##
#set_multicycle_path -setup 5 -from [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_altsyncram_dpm_fifo:U_RTSM|altsyncram*] -to [ get_registers *]
#set_multicycle_path -setup 5 -from [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_retransmit_cntl:U_RETR|*] -to [ get_registers *]
#set_multicycle_path -setup 5 -from [ get_registers *] -to [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_retransmit_cntl:U_RETR|*]
#set_multicycle_path -hold 5 -from [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_altsyncram_dpm_fifo:U_RTSM|altsyncram*] -to [ get_registers *]
#set_multicycle_path -hold 5 -from [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_retransmit_cntl:U_RETR|*] -to [ get_registers *]
#set_multicycle_path -hold 5 -from [ get_registers *] -to [ get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|altera_tse_retransmit_cntl:U_RETR|*]
#set_max_delay 7 -from [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|dout_reg_sft*] -to [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_top_1geth:U_GETH|altera_tse_mac_tx:U_TX|*]
#set_max_delay 7 -from [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|eop_sft*] -to [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_top_1geth:U_GETH|altera_tse_mac_tx:U_TX|*]
#set_max_delay 7 -from [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_tx_min_ff:U_TXFF|sop_reg*] -to [get_registers *|altera_tse_top_w_fifo:U_MAC|altera_tse_top_1geth:U_GETH|altera_tse_mac_tx:U_TX|*]

#
#some clock uncertainty is required
#
derive_clock_uncertainty

# QSPI interface
set_output_delay -clock { clk_50_max10 } -rise -min 11 [get_ports {qspi_io[*]}]
set_output_delay -clock { clk_50_max10 } -rise -min 11 [get_ports {qspi_clk}]
set_output_delay -clock { clk_50_max10 } -rise -min 11 [get_ports {qspi_csn}]
set_input_delay  -clock { clk_50_max10 } -rise -min 10 [get_ports {qspi_io[*]}]

# Ethernet MDIO interface
set_output_delay  -clock [ get_clocks clk_50_max10 ] 2   [ get_ports {enet_mdc} ]
set_input_delay   -clock [ get_clocks clk_50_max10 ] 2   [ get_ports {enet_mdio} ]
set_output_delay  -clock [ get_clocks clk_50_max10 ] 2   [ get_ports {enet_mdio} ]

set_false_path -from [get_ports {fpga_resetn}] 
set_false_path -from [get_ports {user_dipsw[*]}] 
set_false_path -from [get_ports {user_pb[*]}] 
set_false_path -to [get_ports {user_led[*]}]
set_false_path -to [get_ports {enet_resetn}]

set_false_path -from * -to {sld_signaltap:auto_signaltap_0|*}
set_false_path -from {sld_signaltap:auto_signaltap_0|*} -to *
