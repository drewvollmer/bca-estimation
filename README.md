README.md
---------


# Instructions

1. Format the data.  Required columns:
  - AuctionID, a unique integer ID for each auction
  - BidderType, an integer labeling different sets of bidder traits.  No ordinal interpretation is required.
  - OAucType, an integer grouping the auctions into types based on observed traits
  - BidAmount, the quote given in each bid
  - Decision, a 0/1 giving whether that bid was selected as the winner
  - OverallDecision, a 0/1 taking a constant value in the auction and giving whether the procurer selected a bid to win (1) or picked the outside option instead (0)

Other requirements for the data include:
  - Additional variables used to model whether a bid was selected
  - A row for each auction representing the outside option.  It can take value 0 for any bid-specific variables.

2. Select the number of unobserved auction types to be modeled.
Implement as either a command-line argument to the shell file
(e.g. `./run_bca_estimation.sh 3`) or edit the default number of types
in the file.


3. Implement a bid selection model.
  - Change the selection model in `estimate_bid_selection.do`
  - Modify the functions in `bid_selection.cpp` and class definitions in `bid_selection.hpp` to reflect the data format and bid selection model.  The functions to change are getBidData() and getBidSelectionParams() (both used to import data whose format can change) and simulateAuction() (which has utility calculations that change depending on the bid selection model).  The classes to change are Bid and BidSelectionParams, which store information about the bids and the parameters from the selection model.
  - To make it easier to write the functions, you can use the debugging tool `debug_bid_selection.cpp`, which runs each of the defined functions several times.  It can be compiled as `g++ -o debug_bid_selection.exe debug_bid_selection.cpp bid_selection.cpp`

4. Compile the modified version of `calculate_costs.cpp` with the command: `g++ -o calculate_costs.exe calculate_costs.cpp bid_selection.cpp`

5. Run the shell file, or the processes it contains: `./run_bca_estimation.sh [NumUnobsAucTypes]`



# Overview

The estimation has three steps, each one corresponding to a file in
the routine.

1. Separate the auctions by type and find the distribution of bids for
each one: calc_auction_type_probs.m

2. Estimate a model giving the probability that each bid is chosen:
estimate_bid_selection.do

3. Infer bidder costs using the distribution of bids and selection
probabilities: calculate_costs.cpp


# Relation to Yoganarasimhan's Example Code

The file estimate_bid_selection.do is exactly the same (for now) as
the one Yoganarasimhan provided.  I will simplify it later; I have no
intention of presenting her work as my own.

The other two files have been completely rewritten for clarity and
simplicity.  (All of Yoganarasimhan's programs are in CPP, but
calculate_costs.cpp was completely rewritten to add comments and avoid
scoping abuse.)
