#! /bin/bash
# run_bca_estimation.sh
# A wrapper file for the processes associated with the beauty contest auction estimation
# Drew Vollmer 2017-12-18

cd `dirname $0`


# Calculate probabilities for the type of each auction (sortkde3.cc in original code)
calc_auction_type_probs.m

# Calculate bid selection probabilities using nested logit
estimate_bid_selection.do

# Use selection probabilities to solve for bidder costs
calculate_costs.cpp


## This concludes a single iteration of the program.  Full estimation requires more iterations so that we can get standard
## errors and the like.  But the three programs above do the job for a single estimate.
