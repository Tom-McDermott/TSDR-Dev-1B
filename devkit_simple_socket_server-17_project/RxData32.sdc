# clocks from the receive data from ADC
#

create_clock -name "RecvDataClk" -period 8.138 -waveform {0 4.069} [get_ports {RXMCLK}]

create_clock -name RcvDemuxClk -period 16.276 -waveform {0 8.138} [get_pins {rx0|rxwordcnt|q}]

