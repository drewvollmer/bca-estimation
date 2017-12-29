#! /bin/bash
# run_bca_estimation.sh
# A wrapper file for the processes associated with the beauty contest auction estimation
# Drew Vollmer 2017-12-18

cd `dirname $0`

# Iterate auction type probabilities until convergence
sortkde3.m

# Plug converged distributions into Stata for nested logit
synthdata.do

# simulate3-clean.cc using output from sortkde3
./calculate_costs.exe


## This concludes a single iteration of the program.  Full estimation requires more iterations so that we can get standard
## errors and the like.  But the three programs above do the job for a single estimate.
