/*

 DDR_RXData.v
 February 20, 2023
 Tom McDermott, N5EG
 
 Receive 16-bit wide data from the receiver module
 on the rising edge of the 122.88 MHz clock. In the future
 also receive data from the other channel on the falling edge
 of the same clock.
 
 Demultiplex the data to 64-bits wide and generate 30.72 MHz output
 clock.
 
*/

module DDR_RXData(
	output reg [63:0] ch0, // receiver channel 0 output
	output RxClk4,	// 30.72 MHz output clock
	input [15:0] DDR_rx, // DDR data from receiver module (only using half right now)
	input Clk	// DDR input clock
	);


  reg [15:0] rxdat;
  reg [15:0] rx0;
  reg [15:0] rx1;
  reg [15:0] rx2;
  reg [15:0] rx3;
  reg [1:0] rxwordcnt = 0;


  always @(posedge Clk) begin
  
    rxwordcnt <= rxwordcnt + 1;	// demultiplexer word selector
    rxdat <= DDR_rx;		// capture data from the DDR input

    if (rxwordcnt == 0)
    begin
        rx0 <= rxdat;       
	    ch0 [63:0] <= {rx3, rx2, rx1, rx0};
    end
        
    if (rxwordcnt == 1)
        rx1 <= rxdat;

    if (rxwordcnt == 2)
        rx2 <= rxdat;

    if (rxwordcnt == 3)  
        rx3 <= rxdat;     

  end

  assign RxClk4 = !rxwordcnt[1]; 
    
endmodule // DDR_RXData

