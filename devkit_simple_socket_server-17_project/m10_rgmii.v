

module  m10_rgmii (
        //Clock and Reset
        input  wire        clk_50_max10,
        input  wire        fpga_resetn,

        //LED PB DIPSW
        output wire [4:0]  user_led,
        input  wire [3:0]  user_pb,
        input  wire [4:0]  user_dipsw,

        //Dual Ethernet
        output wire        enet_mdc,
        inout  wire        enet_mdio,
        
        //output wire        eneta_resetn,
        //input  wire        eneta_rx_clk,
        //input  wire        eneta_tx_clk,
        //output wire        eneta_gtx_clk,
        //input  wire [3:0]  eneta_rx_d,
        //output wire [3:0]  eneta_tx_d,
        //output wire        eneta_tx_en,
        //input  wire        eneta_rx_dv,
        //input  wire        eneta_led_link100,
        
        //output wire        enetb_resetn,
        //input  wire        enetb_rx_clk,
        //output wire        enetb_gtx_clk,
        //input  wire [3:0]  enetb_rx_d,
        //output wire [3:0]  enetb_tx_d,
        //input  wire        enetb_rx_dv,
        //output wire        enetb_tx_en,
        //input  wire        enetb_led_link100,
		
        output wire        enet_resetn,	
        input  wire        enet_rx_clk,
        input  wire        enet_tx_clk,
        output wire        enet_gtx_clk,
        input  wire [3:0]  enet_rx_d,
        output wire [3:0]  enet_tx_d,
        output wire        enet_tx_en,
        input  wire        enet_rx_dv,
        input  wire        enet_led_link100,
 
        output wire [13:0] mem_a,
        output wire [2:0]  mem_ba,
        inout  wire [0:0]  mem_ck,
        inout  wire [0:0]  mem_ck_n,
        output wire [0:0]  mem_cke,
        output wire [0:0]  mem_cs_n,
        output wire [0:0]  mem_dm,
        output wire [0:0]  mem_ras_n,
        output wire [0:0]  mem_cas_n,
        output wire [0:0]  mem_we_n,
        output wire        mem_reset_n,
        inout  wire [7:0]  mem_dq,
        inout  wire [0:0]  mem_dqs,
        inout  wire [0:0]  mem_dqs_n,
        output wire [0:0]  mem_odt,

        //QSPI
        output          qspi_clk,
        inout  [3:0]    qspi_io,
        output          qspi_csn,
		  
		  // I2C Receiver control
		  inout	wire		rxm_ctrl_scl,
		  inout	wire		rxm_ctrl_sda,
		  
		  // I2C Receiver Ident ROM
		  inout	wire		rxm_id_scl,
		  inout	wire		rxm_id_sda,
		  
		  // I2C Clock C0 control
		  inout	wire		ckm_c0_scl,
		  inout	wire		ckm_c0_sda,
		  
		  // I2C Clock Ident ROM
		  inout	wire		ckm_id_scl,
		  inout	wire		ckm_id_sda,

		  // SPI Receiver ADC
		  inout	wire		rfm_spi_BIDIR,
		  output				rfm_spi_SCLK,
		  output wire		rfm_spi_CSEL_F,
		  
		  // Subclock
		  output 			subclock12_5,
		  
		  // DDR_RXM
		  input	wire		[14:0] DDRRXM,
		  input	wire		RXMCLK

        );

//Heart-beat counter
reg   [25:0]  heart_beat_cnt;

//DDR3 interface assignments
wire          local_init_done;
wire          local_cal_success;
wire          local_cal_fail;

//Ethernet interface assignments
wire          phy_resetn;
wire          system_resetn;

wire          mdio_oen_from_the_tse_mac;
wire          mdio_out_from_the_tse_mac;
wire          eth_mode_from_the_tse_mac;
wire          ena_10_from_the_tse_mac;
wire          enet_tx_125;
wire          enet_tx_25;
wire          enet_tx_2p5;
wire          locked_from_the_enet_pll;
wire          tx_clk_to_the_tse_mac;
wire          tx_clk_to_the_tse_mac_g;

wire          enet_tx_125_shift;
wire          enet_tx_25_shift;
wire          enet_tx_2p5_shift;
wire          enet_tx_250_shift; // signaltap sample clock
wire          locked_from_the_shift_pll;
wire          tx_clk_to_the_tse_mac_shift;
wire          tx_clk_to_the_tse_mac_shift_g;

assign system_resetn = fpga_resetn & local_init_done;

//PHY power-on reset control
parameter MSB = 20; // PHY interface: need minimum 10ms delay for POR
reg [MSB:0] epcount;

always @(posedge clk_50_max10 or negedge fpga_resetn)
  if (!fpga_resetn)
      epcount <= MSB + 1'b0;
  else if (epcount[MSB] == 1'b0)
      epcount <= epcount + 1;
  else
      epcount <= epcount;

assign phy_resetn    = user_pb[0] & !epcount[MSB-1];
assign enet_resetn   = phy_resetn;

//MDIO output control
assign enet_mdio = ( !mdio_oen_from_the_tse_mac ) ? ( mdio_out_from_the_tse_mac ) : ( 1'bz );

assign enet_tx_2p5_shift = !enet_tx_2p5;

//RGMII clock solution
assign tx_clk_to_the_tse_mac = ( eth_mode_from_the_tse_mac ) ? ( enet_tx_125 ) :  // GbE Mode = 125MHz clock
                               ( ena_10_from_the_tse_mac ) ? ( enet_tx_2p5 ) :    // 10Mb Mode = 2.5MHz clock
                               ( enet_tx_25 );                                    // 100Mb Mode = 25MHz clock

assign tx_clk_to_the_tse_mac_shift = ( eth_mode_from_the_tse_mac ) ? ( enet_tx_125_shift ) :  // GbE Mode = 125MHz clock
                                     ( ena_10_from_the_tse_mac ) ? ( enet_tx_2p5_shift ) :    // 10Mb Mode = 2.5MHz clock
                                     ( enet_tx_25_shift);                                     // 100Mb Mode = 25MHz clock

clkctrl             clkctrl_inst0 (
                    .inclk        (tx_clk_to_the_tse_mac  ),
                    .outclk       (tx_clk_to_the_tse_mac_g)
                    );

clkctrl             clkctrl_inst1 (
                    .inclk        (tx_clk_to_the_tse_mac_shift  ),
                    .outclk       (tx_clk_to_the_tse_mac_shift_g)
                    );

enet_gtx_clk_ddio   enet_gtx_clk_ddio_inst (
                    .outclock              (tx_clk_to_the_tse_mac_shift_g), // tx_clk_to_the_tse_mac_g ),
                    .din                   (2'b10                        ),
                    .pad_out               (enet_gtx_clk                 ),
                    .aclr                  (!phy_resetn                  )
                    );

q_sys               q_sys_inst (
                    .sys_clk_clk                                                   (clk_50_max10              ), //                             sys_clk.clk
                    .reset_reset_n                                                 (system_resetn             ), //                               reset.reset_n
                    .mem_resetn_in_reset_reset_n                                   (fpga_resetn               ), //                 mem_resetn_in_reset.reset_n
                    .altpll_shift_c0_clk                                           (enet_tx_250_shift         ), //                     altpll_shift_c0.clk
                    .altpll_shift_locked_conduit_export                            (locked_from_the_shift_pll ), //         altpll_shift_locked_conduit.export
                    .clock_bridge_0_in_clk_clk                                     (enet_tx_25                ), //               clock_bridge_0_in_clk.clk
                    .enet_pll_c0_clk                                               (enet_tx_125               ), //                         enet_pll_c0.clk
                    .enet_pll_c1_clk                                               (enet_tx_25                ), //                         enet_pll_c1.clk
                    .enet_pll_c2_clk                                               (enet_tx_2p5               ), //                         enet_pll_c2.clk
                    .enet_pll_c3_clk                                               (enet_tx_125_shift         ), //                         enet_pll_c3.clk
                    .enet_pll_c4_clk                                               (enet_tx_25_shift          ), //                         enet_pll_c4.clk
                    .enet_pll_locked_conduit_export                                (locked_from_the_enet_pll  ), //             enet_pll_locked_conduit.export
                    .eth_tse_mac_mdio_connection_mdc                               (enet_mdc                  ), //         eth_tse_mac_mdio_connection.mdc
                    .eth_tse_mac_mdio_connection_mdio_in                           (enet_mdio                 ), //                                    .mdio_in
                    .eth_tse_mac_mdio_connection_mdio_out                          (mdio_out_from_the_tse_mac ), //                                    .mdio_out
                    .eth_tse_mac_mdio_connection_mdio_oen                          (mdio_oen_from_the_tse_mac ), //                                    .mdio_oen
                    .eth_tse_mac_rgmii_connection_rgmii_in                         (enet_rx_d                 ), //        eth_tse_mac_rgmii_connection.rgmii_in
                    .eth_tse_mac_rgmii_connection_rgmii_out                        (enet_tx_d                 ), //                                    .rgmii_out
                    .eth_tse_mac_rgmii_connection_rx_control                       (enet_rx_dv                ), //                                    .rx_control
                    .eth_tse_mac_rgmii_connection_tx_control                       (enet_tx_en                ), //                                    .tx_control
                    //.eth_tse_mac_status_connection_set_10                        (user_dipsw[0]             ), //       eth_tse_mac_status_connection.set_10
                    //.eth_tse_mac_status_connection_set_1000                      (user_dipsw[1]             ), //                                    .set_1000
                    .eth_tse_mac_status_connection_set_10                          (                          ), //       eth_tse_mac_status_connection.set_10
                    .eth_tse_mac_status_connection_set_1000                        (                          ), //                                    .set_1000
                    .eth_tse_mac_status_connection_eth_mode                        (eth_mode_from_the_tse_mac ), //                                    .eth_mode
                    .eth_tse_mac_status_connection_ena_10                          (ena_10_from_the_tse_mac   ), //                                    .ena_10
                    .eth_tse_pcs_mac_rx_clock_connection_clk                       (enet_rx_clk               ), // eth_tse_pcs_mac_rx_clock_connection.clk
                    .eth_tse_pcs_mac_tx_clock_connection_clk                       (tx_clk_to_the_tse_mac_g   ), // eth_tse_pcs_mac_tx_clock_connection.clk
                    .ext_flash_flash_dataout_conduit_dataout                       (qspi_io                   ), //             ext_flash_flash_dataout.conduit_dataout
                    .ext_flash_flash_dclk_out_conduit_dclk_out                     (qspi_clk                  ), //            ext_flash_flash_dclk_out.conduit_dclk_out
                    .ext_flash_flash_ncs_conduit_ncs                               (qspi_csn                  ), //                 ext_flash_flash_ncs.conduit_ncs
                    .memory_mem_a                                                  (mem_a                     ), //                              memory.mem_a
                    .memory_mem_ba                                                 (mem_ba                    ), //                                    .mem_ba
                    .memory_mem_ck                                                 (mem_ck                    ), //                                    .mem_ck
                    .memory_mem_ck_n                                               (mem_ck_n                  ), //                                    .mem_ck_n
                    .memory_mem_cke                                                (mem_cke                   ), //                                    .mem_cke
                    .memory_mem_cs_n                                               (mem_cs_n                  ), //                                    .mem_cs_n
                    .memory_mem_dm                                                 (mem_dm                    ), //                                    .mem_dm
                    .memory_mem_ras_n                                              (mem_ras_n                 ), //                                    .mem_ras_n
                    .memory_mem_cas_n                                              (mem_cas_n                 ), //                                    .mem_cas_n
                    .memory_mem_we_n                                               (mem_we_n                  ), //                                    .mem_we_n
                    .memory_mem_reset_n                                            (mem_reset_n               ), //                                    .mem_reset_n
                    .memory_mem_dq                                                 (mem_dq                    ), //                                    .mem_dq
                    .memory_mem_dqs                                                (mem_dqs                   ), //                                    .mem_dqs
                    .memory_mem_dqs_n                                              (mem_dqs_n                 ), //                                    .mem_dqs_n
                    .memory_mem_odt                                                (mem_odt                   ), //                                    .mem_odt
                    .led_pio_external_connection_export                            (user_led[3:0]             ), //         led_pio_external_connection.export
                    .mem_if_ddr3_emif_0_status_local_init_done                     (local_init_done           ), //           mem_if_ddr3_emif_0_status.local_init_done
                    .mem_if_ddr3_emif_0_status_local_cal_success                   (local_cal_success         ), //                                    .local_cal_success
                    .mem_if_ddr3_emif_0_status_local_cal_fail                      (local_cal_fail            ), //                                    .local_cal_fail
						  
						  // Receiver Control
						  .i2c_rxm_ctrl_i2c_serial_sda_in              						  (rxm_ctrl_sda_in), 			  //             i2c_rxm_ctrl_i2c_serial.sda_in
						  .i2c_rxm_ctrl_i2c_serial_scl_in              						  (rxm_ctrl_scl_in), 			  //                                    .scl_in
						  .i2c_rxm_ctrl_i2c_serial_sda_oe              						  (rxm_ctrl_sda_oe), 			  //                                    .sda_oe
						  .i2c_rxm_ctrl_i2c_serial_scl_oe              						  (rxm_ctrl_scl_oe),  			  //                                    .scl_oe

						  // Receiver Ident Rom
						  .i2c_rxm_id_i2c_serial_sda_in                						  (rxm_id_sda_in),              //              i2c_rxm_id_i2c_serial.sda_in
						  .i2c_rxm_id_i2c_serial_scl_in                						  (rxm_id_scl_in),              //                                   .scl_in
						  .i2c_rxm_id_i2c_serial_sda_oe                						  (rxm_id_sda_oe),              //                                   .sda_oe
						  .i2c_rxm_id_i2c_serial_scl_oe                						  (rxm_id_scl_oe),              //                                   .scl_oe

						  // Clock C0 (Clock C1 is not used)
						  .i2c_ckm_c0_i2c_serial_sda_in                						  (ckm_c0_sda_in),              //               i2c_ckm_c0_i2c_serial.sda_in
						  .i2c_ckm_c0_i2c_serial_scl_in                						  (ckm_c0_scl_in),              //                                    .scl_in
						  .i2c_ckm_c0_i2c_serial_sda_oe                						  (ckm_c0_sda_oe),              //                                    .sda_oe
						  .i2c_ckm_c0_i2c_serial_scl_oe                						  (ckm_c0_scl_oe),              //                                    .scl_oe
						  
						  // Clock Module Ident Rom
						  .i2c_ckm_id_i2c_serial_sda_in                						  (ckm_id_sda_in),              //               i2c_ckm_id_i2c_serial.sda_in
						  .i2c_ckm_id_i2c_serial_scl_in                						  (ckm_id_scl_in),              //                                    .scl_in
						  .i2c_ckm_id_i2c_serial_sda_oe                						  (ckm_id_sda_oe),              //                                    .sda_oe
						  .i2c_ckm_id_i2c_serial_scl_oe                						  (ckm_id_scl_oe),              //                                    .scl_oe
						  
						  // RF Module ADC SPI
						  .spi_rxm_external_MISO                       						  (rfm_spi_MISO),			        //                    spi_rxm_external.MISO
						  .spi_rxm_external_MOSI                       						  (rfm_spi_MOSI),       		  //                                    .MOSI
						  .spi_rxm_external_SCLK                       						  (rfm_spi_SCLK),        		  //                                    .SCLK
						  .spi_rxm_external_SS_n                       						  (rfm_spi_CSEL_F),       		  //                                    .SS_n
						  
						  // RF Module RX data and clock 
						  .clock_bridge_1_in_clk_clk                   						  (dmuxclk),                    //                 clock_bridge_1_in_clk.clk
						  .fifo_0_in_valid                             						  (dmuxvalid),                  //                           fifo_0_in.valid
						  .fifo_0_in_data                              						  (dmuxdata),                   //                                    .data
						  .fifo_0_in_ready                             						  (fifoready),                  //                                    .ready

						  // CPU PIO to rest RAM-FIFO
						  .pio_fifo_external_connection_export         						  (RAMFIFO_reset),         	  //        pio_fifo_external_connection.export
						  .reset_bridge_fifo_in_reset_reset            						  (RAMFIFO_reset)               //          reset_bridge_fifo_in_reset.reset

                    );

//Heart beat by 50MHz clock
always @(posedge clk_50_max10 or negedge fpga_resetn)
  if (!fpga_resetn)
      heart_beat_cnt <= 26'h0; //0x3FFFFFF
  else
      heart_beat_cnt <= heart_beat_cnt + 1'b1;

//assign user_led[0] = !locked_from_the_enet_pll;
//assign user_led[1] = !ena_10_from_the_tse_mac;
//assign user_led[2] = !eth_mode_from_the_tse_mac;
//assign user_led[3] = !local_init_done | local_cal_fail;
assign user_led[4] = heart_beat_cnt[25];

//`include "Tangerine.h"	// instantiate the Tangerine module

// Instantiate just the rxm_ctrl i2c interface for now

// Receiver control I2C
wire	rxm_ctrl_sda_oe;
wire  rxm_ctrl_scl_oe;
wire	rxm_ctrl_sda_in;
wire  rxm_ctrl_scl_in;

I2CBUF i2crxmctrl (
	.sda_oe	(rxm_ctrl_sda_oe),
	.sda		(rxm_ctrl_sda),
	.sda_in	(rxm_ctrl_sda_in),
	.scl_oe	(rxm_ctrl_scl_oe),
	.scl		(rxm_ctrl_scl),
	.scl_in	(rxm_ctrl_scl_in)
	);

	
// Receiver IDent Prom
wire	rxm_id_sda_oe;
wire  rxm_id_scl_oe;
wire	rxm_id_sda_in;
wire  rxm_id_scl_in;

I2CBUF i2crxmid (
	.sda_oe	(rxm_id_sda_oe),
	.sda		(rxm_id_sda),
	.sda_in	(rxm_id_sda_in),
	.scl_oe	(rxm_id_scl_oe),
	.scl		(rxm_id_scl),
	.scl_in	(rxm_id_scl_in)
	);
	

// Clock Module I2C C0
wire	ckm_c0_sda_oe;
wire  ckm_c0_scl_oe;
wire	ckm_c0_sda_in;
wire  ckm_c0_scl_in;

I2CBUF i2cckmc0 (
	.sda_oe	(ckm_c0_sda_oe),
	.sda		(ckm_c0_sda),
	.sda_in	(ckm_c0_sda_in),
	.scl_oe	(ckm_c0_scl_oe),
	.scl		(ckm_c0_scl),
	.scl_in	(ckm_c0_scl_in)
	);	

// Clock Module Ident Prom
wire	ckm_id_sda_oe;
wire  ckm_id_scl_oe;
wire	ckm_id_sda_in;
wire  ckm_id_scl_in;

I2CBUF i2cckmid (
	.sda_oe	(ckm_id_sda_oe),
	.sda		(ckm_id_sda),
	.sda_in	(ckm_id_sda_in),
	.scl_oe	(ckm_id_scl_oe),
	.scl		(ckm_id_scl),
	.scl_in	(ckm_id_scl_in)
	);	


subclock SC (
	.clock1	(clk_50_max10),
	.clock4	(subclock12_5)
	);
	
	
// RF Module ADC SPI
wire	rfm_spi_MOSI;
wire	rfm_spi_MISO;

SPIConverter rfmadc (
	.resetn	(fpga_resetn),
	.MRx 		(rfm_spi_MISO),
	.TSMTx 	(rfm_spi_BIDIR),
	.MTx		(rfm_spi_MOSI),
	.Sclk		(rfm_spi_SCLK),
	.Cs_f		(rfm_spi_CSEL_F)
	);

// Receive data from the (only) receiver
// fanned out to 32 bit wide double word

wire [31:0] dmuxdata;
wire dmuxclk;
wire dmuxvalid;
wire fifoready;

wire RAMFIFO_reset;
//
RXData rx0 (
	.ch1		(dmuxdata),  // receiver channel 0 dmux output
	.RxClk2	(dmuxclk),	 // 61.44 MHz clock out
	.RXMCH0_dat (DDRRXM), // 14 bit rx data + overrange from ADC
	.Clk		(RXMCLK),	 // DC0A rx data clk from ADC
	.Ready 	(fifoready), // FIFO is ready
	.Valid 	(dmuxvalid)	 // Data to FIFO is Valid
	);
	
endmodule


