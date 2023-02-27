/*

SPIconverter_tb.v
September 22, 2022
Tom McDermott, N5EG

Test the SPIconverter module. Emulate some
commands emitted by the SPI Master on the FPGA

*/

`timescale 1 ns / 1 ns

module test;

initial
 begin
 	$dumpfile("test.vcd");
 	$dumpvars(0, test);
 end

  reg MTx = 1;
  reg CSf = 1;  	// SPI Chipselect.
  
  initial begin
     # 7 CSf = 0;
     # 50 CSf = 1;
     # 90 CSf = 0;
     # 300 CSf = 1;
     # 120 MTx = 0;
     # 160 CSf = 0;
     # 400 CSf = 1;
     
     # 1450 $stop;
  end
  


  /* SPI Clk - falling edge clocks FFs. */
  reg sclk = 1;
  always #5 sclk = !sclk;


  SPIConverter s1 (MRx, TSMtx, MTx, sclk, CSf);


endmodule // test

