
module q_sys (
	altpll_shift_c0_clk,
	altpll_shift_locked_conduit_export,
	clock_bridge_0_in_clk_clk,
	clock_bridge_1_in_clk_clk,
	enet_pll_c0_clk,
	enet_pll_c1_clk,
	enet_pll_c2_clk,
	enet_pll_c3_clk,
	enet_pll_c4_clk,
	enet_pll_locked_conduit_export,
	eth_tse_mac_mdio_connection_mdc,
	eth_tse_mac_mdio_connection_mdio_in,
	eth_tse_mac_mdio_connection_mdio_out,
	eth_tse_mac_mdio_connection_mdio_oen,
	eth_tse_mac_rgmii_connection_rgmii_in,
	eth_tse_mac_rgmii_connection_rgmii_out,
	eth_tse_mac_rgmii_connection_rx_control,
	eth_tse_mac_rgmii_connection_tx_control,
	eth_tse_mac_status_connection_set_10,
	eth_tse_mac_status_connection_set_1000,
	eth_tse_mac_status_connection_eth_mode,
	eth_tse_mac_status_connection_ena_10,
	eth_tse_pcs_mac_rx_clock_connection_clk,
	eth_tse_pcs_mac_tx_clock_connection_clk,
	ext_flash_flash_dataout_conduit_dataout,
	ext_flash_flash_dclk_out_conduit_dclk_out,
	ext_flash_flash_ncs_conduit_ncs,
	fifo_0_in_valid,
	fifo_0_in_data,
	fifo_0_in_ready,
	i2c_ckm_c0_i2c_serial_sda_in,
	i2c_ckm_c0_i2c_serial_scl_in,
	i2c_ckm_c0_i2c_serial_sda_oe,
	i2c_ckm_c0_i2c_serial_scl_oe,
	i2c_ckm_id_i2c_serial_sda_in,
	i2c_ckm_id_i2c_serial_scl_in,
	i2c_ckm_id_i2c_serial_sda_oe,
	i2c_ckm_id_i2c_serial_scl_oe,
	i2c_rxm_ctrl_i2c_serial_sda_in,
	i2c_rxm_ctrl_i2c_serial_scl_in,
	i2c_rxm_ctrl_i2c_serial_sda_oe,
	i2c_rxm_ctrl_i2c_serial_scl_oe,
	i2c_rxm_id_i2c_serial_sda_in,
	i2c_rxm_id_i2c_serial_scl_in,
	i2c_rxm_id_i2c_serial_sda_oe,
	i2c_rxm_id_i2c_serial_scl_oe,
	led_pio_external_connection_export,
	mem_if_ddr3_emif_0_status_local_init_done,
	mem_if_ddr3_emif_0_status_local_cal_success,
	mem_if_ddr3_emif_0_status_local_cal_fail,
	mem_resetn_in_reset_reset_n,
	memory_mem_a,
	memory_mem_ba,
	memory_mem_ck,
	memory_mem_ck_n,
	memory_mem_cke,
	memory_mem_cs_n,
	memory_mem_dm,
	memory_mem_ras_n,
	memory_mem_cas_n,
	memory_mem_we_n,
	memory_mem_reset_n,
	memory_mem_dq,
	memory_mem_dqs,
	memory_mem_dqs_n,
	memory_mem_odt,
	reset_reset_n,
	spi_rxm_external_MISO,
	spi_rxm_external_MOSI,
	spi_rxm_external_SCLK,
	spi_rxm_external_SS_n,
	sys_clk_clk,
	pio_fifo_external_connection_export,
	reset_bridge_fifo_in_reset_reset);	

	output		altpll_shift_c0_clk;
	output		altpll_shift_locked_conduit_export;
	input		clock_bridge_0_in_clk_clk;
	input		clock_bridge_1_in_clk_clk;
	output		enet_pll_c0_clk;
	output		enet_pll_c1_clk;
	output		enet_pll_c2_clk;
	output		enet_pll_c3_clk;
	output		enet_pll_c4_clk;
	output		enet_pll_locked_conduit_export;
	output		eth_tse_mac_mdio_connection_mdc;
	input		eth_tse_mac_mdio_connection_mdio_in;
	output		eth_tse_mac_mdio_connection_mdio_out;
	output		eth_tse_mac_mdio_connection_mdio_oen;
	input	[3:0]	eth_tse_mac_rgmii_connection_rgmii_in;
	output	[3:0]	eth_tse_mac_rgmii_connection_rgmii_out;
	input		eth_tse_mac_rgmii_connection_rx_control;
	output		eth_tse_mac_rgmii_connection_tx_control;
	input		eth_tse_mac_status_connection_set_10;
	input		eth_tse_mac_status_connection_set_1000;
	output		eth_tse_mac_status_connection_eth_mode;
	output		eth_tse_mac_status_connection_ena_10;
	input		eth_tse_pcs_mac_rx_clock_connection_clk;
	input		eth_tse_pcs_mac_tx_clock_connection_clk;
	inout	[3:0]	ext_flash_flash_dataout_conduit_dataout;
	output		ext_flash_flash_dclk_out_conduit_dclk_out;
	output	[0:0]	ext_flash_flash_ncs_conduit_ncs;
	input		fifo_0_in_valid;
	input	[31:0]	fifo_0_in_data;
	output		fifo_0_in_ready;
	input		i2c_ckm_c0_i2c_serial_sda_in;
	input		i2c_ckm_c0_i2c_serial_scl_in;
	output		i2c_ckm_c0_i2c_serial_sda_oe;
	output		i2c_ckm_c0_i2c_serial_scl_oe;
	input		i2c_ckm_id_i2c_serial_sda_in;
	input		i2c_ckm_id_i2c_serial_scl_in;
	output		i2c_ckm_id_i2c_serial_sda_oe;
	output		i2c_ckm_id_i2c_serial_scl_oe;
	input		i2c_rxm_ctrl_i2c_serial_sda_in;
	input		i2c_rxm_ctrl_i2c_serial_scl_in;
	output		i2c_rxm_ctrl_i2c_serial_sda_oe;
	output		i2c_rxm_ctrl_i2c_serial_scl_oe;
	input		i2c_rxm_id_i2c_serial_sda_in;
	input		i2c_rxm_id_i2c_serial_scl_in;
	output		i2c_rxm_id_i2c_serial_sda_oe;
	output		i2c_rxm_id_i2c_serial_scl_oe;
	output	[3:0]	led_pio_external_connection_export;
	output		mem_if_ddr3_emif_0_status_local_init_done;
	output		mem_if_ddr3_emif_0_status_local_cal_success;
	output		mem_if_ddr3_emif_0_status_local_cal_fail;
	input		mem_resetn_in_reset_reset_n;
	output	[13:0]	memory_mem_a;
	output	[2:0]	memory_mem_ba;
	inout	[0:0]	memory_mem_ck;
	inout	[0:0]	memory_mem_ck_n;
	output	[0:0]	memory_mem_cke;
	output	[0:0]	memory_mem_cs_n;
	output	[0:0]	memory_mem_dm;
	output	[0:0]	memory_mem_ras_n;
	output	[0:0]	memory_mem_cas_n;
	output	[0:0]	memory_mem_we_n;
	output		memory_mem_reset_n;
	inout	[7:0]	memory_mem_dq;
	inout	[0:0]	memory_mem_dqs;
	inout	[0:0]	memory_mem_dqs_n;
	output	[0:0]	memory_mem_odt;
	input		reset_reset_n;
	input		spi_rxm_external_MISO;
	output		spi_rxm_external_MOSI;
	output		spi_rxm_external_SCLK;
	output		spi_rxm_external_SS_n;
	input		sys_clk_clk;
	output		pio_fifo_external_connection_export;
	input		reset_bridge_fifo_in_reset_reset;
endmodule
