/*

 SPIconverter.v
 December 15, 2022
 Tom McDermott, N5EG
 
 Convert 4-wire SPI Master to 3-wire SPI slave.
 Sets data output active from FPGA when idle or writing.
 When reading, sets output to z and reads 8 bits of
 input from slave. Master clock changes outputs on SPI_clk falling
 edge. Target and Maaster read bits on rising edge.
 
 This version revised based on observed behavior of Qsys SPI Master,
 which is different than shown on Analog Devices App Note 877.
 Sclk is gated, cannot use as a sync clock.  Need async resets
 to clear states.

*/

module SPIConverter(
	input resetn, 	// Power on resetn
	output MRx, 	// SPI data into our Master (output from uconverter)
	inout TSMTx, 	// SPI data in/out of the FPGA tristate
	input MTx,	// SPI data out from Master
	input Sclk,	// SPI clock (nominally 6.25 MHz)
	input Cs_f	// SPI chip select false
	);


	reg [4:0] bitcount;
	reg readcommand;

	always @(negedge Sclk or posedge Cs_f or negedge resetn)
	begin
	   if (Cs_f || ~resetn)
			bitcount <= 0;
		else
			bitcount <= bitcount + 1;
	end
	
	always @(posedge Sclk)
	begin
		if (bitcount == 0 && MTx)
			readcommand <= 1;
		if (bitcount == 0 && ~MTx)
			readcommand <= 0;
	end

   assign TSMTx = (bitcount[4] && readcommand) ? 1'bz : MTx;  
   assign MRx = TSMTx;   // When output is Z should read the pin like an input
    
endmodule // SPIconverter

