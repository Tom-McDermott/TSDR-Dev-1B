# Tangerine SDR Development Project

## This is a developer repository for the TangerineSDR FPGA and C code. This version is intended for the MAX10 Development Board, the TAPR DE_Adapter, Receiver Module, and Clock Module.

### A new repository will be created when the Data Engine replaces the FPGA Development Kit + Adapter board.

-------------------------------
The Verilog and C software at this time is pretty much guaranteed not to work. The current contents have not yet passed Test Stage 1A due to errors. At this point:

1. Gigabit Ethernet interface works with a hard-coded MAC address and IPv4. It must use DHCP and cannot be set for a static address.

2. OpenHPSDR discovery and Provisioning Channel creation works.

3. Provising Channel accepts a limited subset of the LH_DE commands.

**Prerequisites:** You will need Quartus version 20.1 installed under a Linux OS (probably Centos 7) to sucessfully build. The Verilog for DDR3 memory does not correctly synthesize code on the associated Windows version.

Read the Wierd_stuff.txt file for Quartus and NIOS strangeness related to this project. You WILL have to edit some files before rebuilding the FPGA image.

If you are not familiar with Quartus, the learning curve is very steep and high enough that supplemental oxygen might be needed.

-----------------------

To initially open the project with Quartus, use File --> Open then navigate to
<wherever_installed>/TSDR-Dev/devkit_simple_socket_server-17_project/top.qpf
The file selector does not show qpf files, so you'll need to type in top.qpf
when you get to the correct directory in the file selector.

-----------------------

When you use the NIOS / Eclipse software development tool, specify the directory to use as: /TSDR-Dev/devkit_simple_socket_server-17_project/software

