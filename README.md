# An example of using Ramulator as memory model in a cycle-accurate SystemC Design
SystemC which is a C++ library for parallel hardware description is efficient 
for developing cycle-accurate hardware accelerators. While 
memory is critical to many hardware accelerator design and delicate memory access 
is required for the sake of performance as well as other design trade-offs,there are few open 
source cycle-accurate DDR models immediately available for SystemC.  
Myoungsoo \[6\] developed a SystemC interface on top of DRAMSim2 
which has limited DDR models supported. Ramulator is ralative new, and it  a fast 
and cycle-accurate DRAM simulator \[1\] that supports a wide array of commercial, as well as 
academic, DRAM standards:

- DDR3 (2007), DDR4 (2012)
- LPDDR3 (2012), LPDDR4 (2014)
- GDDR5 (2009)
- WIO (2011), WIO2 (2014)
- HBM (2013)
- SALP \[2\]
- TL-DRAM \[3\]
- RowClone \[4\]
- DSARP \[5\]

[\[1\] Kim et al. *Ramulator: A Fast and Extensible DRAM Simulator.* IEEE CAL
2015.](https://users.ece.cmu.edu/~omutlu/pub/ramulator_dram_simulator-ieee-cal15.pdf)  
[\[2\] Kim et al. *A Case for Exploiting Subarray-Level Parallelism (SALP) in
DRAM.* ISCA 2012.](https://users.ece.cmu.edu/~omutlu/pub/salp-dram_isca12.pdf)  
[\[3\] Lee et al. *Tiered-Latency DRAM: A Low Latency and Low Cost DRAM
Architecture.* HPCA 2013.](https://users.ece.cmu.edu/~omutlu/pub/tldram_hpca13.pdf)  
[\[4\] Seshadri et al. *RowClone: Fast and Energy-Efficient In-DRAM Bulk Data
Copy and Initialization.* MICRO
2013.](https://users.ece.cmu.edu/~omutlu/pub/rowclone_micro13.pdf)  
[\[5\] Chang et al. *Improving DRAM Performance by Parallelizing Refreshes with
Accesses.* HPCA 2014.](https://users.ece.cmu.edu/~omutlu/pub/dram-access-refresh-parallelization_hpca14.pdf)  
[\[6\] Myoungsoo Jung. *SCIC: A System C Interface Converter for DRAMSim.* 2011.](https://github.com/LBNL-CODEX/DRAMSim_SystemC)

With Ramulator we may explore hardware accelerators on many state-of-art memory models.
As it is initially devloped for memory architecture research and can't be used directly in a 
SystemC design, we aim to integrate ramulator in a SystemC design in this project. To utilize the 
Ramulator as a memory model in a SystemC design, we mainly solved the following problems:

1) Both Ramulator and SystemC have its own timing management, we basically pack the ramulator 
as a SystemC thread and have it synchronized to the SystemC design. 

2) Ramulator provides only latency information but no memory content management 
which is needed in many accelerator design. In this project, we keep the memory as 
a dynamic vector and maintain the memory content based on the received memory 
request sequence. Eventually the memory complies with a sequential consistency model.

3) Ramulator provides only basic memory request i.e. each memory request operates on 
a determined aligned length of data which is 64-byte in most cases. This is 
not the usual case for hardware accelerator design which has diverse burst transmissions. 
In this work, we developed a memory wrapper that provides arbitrary burst memory 
access interface. In addition, we also demonstrate how to have multiple parallel 
memory access interfaces share the same memory model.

4) Finally, we developed a vector addition accelerator as an example demonstrating the 
use of Ramulator in a SystemC based accelerator.

## Getting Started
[SystemC-2.3.1](http://accellera.org/downloads/standards/systemc) 
is used for the accelerator design. You need to download and compile it first. Then 
you may change the SystemC library path accordingly in the Makefile. 
In order to compile Ramulator, a C++11 compiler (e.g., `clang++`, `g++-5`) is required. 
You may refer to [Ramulator git repo](https://github.com/CMU-SAFARI/ramulator) 
for more details. When the compilation environment is ready, you can compile 
and run the example using the following commands.   

$ cd vecadd-over-ramulator  
$ make   
$ make exe   
       
## Simulation Output
To be added.

### Contributors
- Cheng Liu (National University of Singapore) 
