# CppRiscVEmulator
An attempt at writing my first proper emulator. 
This project aims to emulate a Risc-V processor,
Aiming to emulate a RV64IG processor on AMD64 (x86_64) systems

### Aims
* To emulate a Risc-V processor using the 64-bit Base Integer Instruction Set
* Add support for the following extensions:
  - M - Integer Multiplication and Division
  - A - Atomic Instructions
  - F - Single-Precision Floating-Point
  - D - Double-Precision Floating-Point

Support for all of these extensions comes under the extension G

As such the processor I am seeking to emulate is an RV64IG, 
RV64I for the 64-bit base and G for all of the extensions.

#### Notes for myself :)

* full implementation of RV64GCV would allow for most general computing 
use-cases
    - C - Compressed Instructions
    - V - Vector operations
* Implementing H (Hypervisor instructions) could be interesting
* However, at first RV64I should be the main goal
* A useful resource for aiding in emulator development: 
http://fms.komkon.org/EMUL8/HOWTO.html
 