/*

Sub-system clock generator. Intended to clock Signal Tap at a lower
rate than 50 MHz as otherwise stuff seems to break. 



*/

module subclock(
	input clock1,
	output clock4
	);
	
	reg[1:0] count;
	
	always @(posedge clock1) begin
	
		count <= count +1;
	
	end
	
	assign clock4 = count[0];
	
endmodule
		
		