# An example of using Ramulator as memory model in a cycle-accurate SystemC Design
SystemC is convenient for developing cycle-accurate hardware accelerators. However, many 
hardware accelerators require delicate memory access for the sake of 
performance while there are few open source cycle-accurate DDR models immediately 
available for SystemC. Myoungsoo \[6\] developed a SystemC interface on top of DRAMSim2 
which has limited DDR models supported. Ramulator is a fast and cycle-accurate 
DRAM simulator \[1\] that supports a wide array of commercial, as well as 
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
[\[6\] Myoungsoo Jung. *SCIC: A System C Interface Converter for DRAMSim.* 2011.] (https://github.com/LBNL-CODEX/DRAMSim_SystemC)

Thus we use it for the cycle-accurate memory model in SystemC design. 
In order to integrate ramulator for SystemC based hardware accelerator design, 
we mainly solved the following problems in this project.

1) Both Ramulator and SystemC have its own timing management, we basically pack the ramulator 
as a SystemC thread and have it synchronized to the SystemC design. 

2) Ramulator provides only latency information but no memory content management 
which is needed in many accelerator design. In this project, we keep the memory as 
a dynamic vector and maintain the memory content based on sequential memory consistency.

3) Ramulator provides only basic memory request i.e. each memory request operates on 
a determined aligned length of data which is 64-byte in most cases. This is 
not convenient for hardware accelerator design which has diverse burst transmission. 
In this work, we developed a memory wrapper that provides arbitrary burst memory access.

4) Finally, we developed a vector addition accelerator as an example. The users 
may work on top of it for your own design.

Also note that we also change part of the Ramulator source code (mostly the Request.h). 

## Getting Started
Ramulator requires a C++11 compiler (e.g., `clang++`, `g++-5`).
You may refer to [Ramulator git repo](https://github.com/CMU-SAFARI/ramulator) for more information about the details. In this project, we use [SystemC-2.3.1](http://accellera.org/downloads/standards/systemc) as the library. You need to download and compile it first. Then 
you may change the SystemC library path accordingly in the Makefile. 

$ cd ramulator
$ make 
$ make exe 
       
## Simulation Output
To be added.

### Contributors
- Cheng Liu (National University of Singapore) 
