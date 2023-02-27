/*

 SPIconverter.v
 September 22, 2022
 Tom McDermott, N5EG
 
 Convert 4-wire SPI Master to 3-wire SPI slave.
 Sets data output active from FPGA when idle or writing.
 When reading, sets output to z and reads 8 bits of
 input from slave. Master clocks outputs on SPI_clk falling
 edge, and samples bits on rising edge.
 .
*/

module SPIConverter(
	output MRx, 	// SPI data into our Master (output from uconverter)
	inout TSMTx, 	// SPI data in/out of the FPGA tristate
	input MTx,	// SPI data out from Master
	input Sclk,	// SPI clock (nominally 6.25 MHz)
	input Cs_f	// SPI chip select false
	);


  reg CsDel_f;
  reg [4:0] bitcount;
//  wire OutEnab;


  always @(negedge Sclk) begin
  
    CsDel_f <= Cs_f;	// one clock delayed chip select
    
    if (Cs_f | bitcount == 23)
      bitcount <= 0;
    else if (CsDel_f & !Cs_f & ~MTx) // start write command
        bitcount <= 0;
    else if (CsDel_f & !Cs_f & MTx) // start read command
        bitcount <= 1;
    else if (bitcount > 0)
    	bitcount <= bitcount + 1;
    else bitcount <= 0;
   
  end

    assign TSMTx = (bitcount[4]) ? 1'bz : MTx;  
    assign MRx = TSMTx;   // When output is Z should read the pin like an input
    
endmodule // SPIconverter

