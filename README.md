README.md
---------


# Instructions for Running

- Specify the number of unobserved auction types as a command-line
argument to the shell file, or by editing the default number of types
in the file

- Specify the desired selection model in estimate_bid_selection.do

- Modify the selection probability formula and derivative in
  calculate_costs.cpp to match the selection model

- Compile calculate_costs.cpp if edited: g++ calculate_costs.cpp -o
  calculate_costs.exe

- Run shell file, or the sequence of processes contained in it



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
