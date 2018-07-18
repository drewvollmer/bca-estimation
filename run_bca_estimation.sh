#! /bin/bash
# run_bca_estimation.sh
# A wrapper file for the processes associated with the beauty contest auction estimation
# Drew Vollmer 2017-12-18

# Run the script from the directory where it's located
cd `dirname $0`

# Delete files from earlier runs (which would interfere with auction parameter inferences in calculate_costs.cpp)
rm inv_cdf*.csv
rm costs.csv
rm coeff.txt
rm unobs_auc_type_probs.csv


# Calculate the distribution of bids for each type of auction, taking
# the number of unobserved auction types as a parameter.  The
# parameter is given at the command line with three types as the
# default.
if [[ $# == 1 ]] ; then
    echo "Running for $1 unobserved auction types."
    matlab -nodisplay -nosplash -r "try, calc_auction_type_probs($1), catch, end, quit"
else
    echo "No number of unobserved auction types given.  Running for default of 1 (no unobserved types)."
    matlab -nodisplay -nosplash -r "try, calc_auction_type_probs(1), catch, end, quit"
fi


# Calculate bid selection probabilities using nested logit
# This can be run without the auction types calculated in estimate_auction_type_probs.m
estimate_bid_selection.do

# Sample bids for each type of bidder in each type of auction
matlab -nodisplay -nosplash -r sample_bids.m

# Use selection probabilities to solve for bidder costs
# To compile:
# g++ -o calculate_costs.exe calculate_costs.cpp bid_selection.cpp
./calculate_costs.exe


## TODO
## Implement sample_bids.m, or keep it as a wrapper to be filled in later
## Change calculate_costs.exe to sample bids from the files of sample bids for each type
