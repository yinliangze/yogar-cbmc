
           Yogar-CBMC v0.1 
			     Order Graph based Abstract Refinement for Multi-threaded Program Verification 
                 October 2016

* Installation *

Same with CBMC, see Yogar-CBMC/COMPILING for more detail

* Usage (SV-COMP 2017) *

To run Yogar-CBMC, use the following command-line from this directory:

$./yogar-cbmc --no-unwinding-assertions --32 --error-label ERROR --mm sc <inputfile>

where  <inputfile>  is the filename to be verified. 

The counterexample will also be written in counterexample.witness.
