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


# Calculate the distribution of bids for each type of auction, taking
# the number of unobserved auction types as a parameter.  The
# parameter is given at the command line with three types as the
# default.
if [[ $# == 1 ]] ; then
    echo "Running for $1 unobserved auction types."
    matlab -nodisplay -nosplash -r "try, calc_auction_type_probs($1), catch, end, quit"
else
    echo "No number of unobserved auction types (or too many) given.  Running for default of 3."
    matlab -nodisplay -nosplash -r "try, calc_auction_type_probs(3), catch, end, quit"
fi


# Calculate bid selection probabilities using nested logit
# This can be run without the auction types calculated in estimate_auction_type_probs.m
estimate_bid_selection.do

# Use selection probabilities to solve for bidder costs
# To compile:
# g++ -o calculate_costs.exe calculate_costs.cpp bid_selection.cpp
./calculate_costs.exe
