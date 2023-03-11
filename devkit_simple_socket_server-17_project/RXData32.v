/*

 RXData32.v
 March 9, 2023
 Tom McDermott, N5EG
 
 Receive 16-bit wide data from the receiver module from CMOS
 on one channel on the rising edge of the 122.88 MHz DCOA clock.
 In the future also receive data from the other channel on the
 DCOB clock.
 
 Demultiplex the data to 32-bits wide and generate 61.44 MHz output
 clock.
 
*/

module RXData(
	output reg [31:0] ch1, // receiver channel 0 output
	output RxClk2,	// 61.44 MHz output clock
	input [15:0] RXMCH0_dat, // RXM data from receiver module (only using half right now)
	input Clk,	// DCOA input clock
	input Ready,
	output reg Valid
	);


  reg [15:0] rxdat;
  reg [15:0] rx0;
  reg [15:0] rx1;
  reg rxwordcnt = 0;
  reg [31:0] ch0;
//  reg xfer;


  always @(posedge Clk) begin
  
    rxwordcnt <= rxwordcnt + 1;	// demultiplexer word selector
    rxdat <= RXMCH0_dat;		// capture data from the DDR input

    if (rxwordcnt == 0)
    begin
        rx0 <= rxdat;  
        ch0 <= { rx1, rx0 };
    end
        
    if (rxwordcnt == 1)
        rx1 <= rxdat;
          
  end
  
  always @(negedge rxwordcnt) begin
  	Valid <= Ready;
  	ch1 <= ch0;
  end
  
  assign RxClk2 = !rxwordcnt;

 
    
endmodule // DDR_RXData

