#! /bin/bash
# run_bca_estimation.sh
# A wrapper file for the processes associated with the beauty contest auction estimation
# Drew Vollmer 2017-12-18

cd `dirname $0`

# Delete files from earlier runs (which would interfere with auction parameter inferences in calculate_costs.cpp)
rm inv_cdf*.csv

# Calculate probabilities for the type of each auction (sortkde3.cc in original code)
# (Note the command line is the number of auction types in the model)
matlab -nodisplay -nosplash -r "try, estimate_auction_type_probs(3), catch, end, quit"

# Calculate bid selection probabilities using nested logit
# This can be run without the auction types calculated in estimate_auction_type_probs.m
estimate_bid_selection.do

# Use selection probabilities to solve for bidder costs
# Compile if necessary
# g++ calculate_costs.cpp -o calculate_costs.exe
./calculate_costs.exe

