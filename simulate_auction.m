% simulate_auction.m
% A function separating the auction simulation repeated to find seller costs
% Given: details of the bid and the type of auction
% Drew Vollmer 2017-12-20

function [prob, prob_deriv] = simulate_auction(bid, auction_type, nlogit_params, inv_cdfs)

%% 1. Get 9 competing bids
% Start by drawing their bidder types (i.e. seller ratings)
%% Buyer types and number of other bids are hard-coded here
btypes = randi([1 3], 9, 1);
%% Note that this assumes the three types of bidders are uniformly distributed. Will need to change
%% that for the full estimation.

% icdfs is an inverse CDF. It takes in a quantile and returns a bid
% Take a random uniform draw on [0, 1]. Then find the bid corresponding to that amount of tail
% probability in the relevant CDF
% It probably is more efficient to calculate the inverse CDF!

% Take 9 uniform draws for competing bids
bid_quantiles = randi([1 1000], 9, 1);



% Get bids
bids = inv_cdfs( sub2ind( size(inv_cdfs), btypes, (4 - auction_type)*ones(9, 1), bid_quantiles ) );
% This matches draws in simulate3-clean.cc: inv_cdf(btype, 4 - type, quantile) works. (type = 1
% corresponds to the third type in my CDF.)
%% So: is my inverse CDF matrix indexed wrong?  Is the 4 - correction necessary?

% %% Debugging: set bids and bidder types manually
% bids = [246.528, 218.8331, 261.0508, 250.574, 257.6782, 200.9693, 264.1822, 261.9003, 185.5633]';
% btypes = [2, 0, 2, 0, 1, 1, 2, 2, 0]' + 1;



% Evaluate selection probability and derivative
utilities = ( nlogit_params.c6_price*[bid.bidAmount; bids] + nlogit_params.c6_sellrep*([bid.btype; btypes] - 1) ) ...
    / nlogit_params.nestCorr;

% Get inclusive value
incl_value = log( sum(exp( utilities - max(utilities) )) ) + max(utilities);

%% lnnumreps, buyrep, and prevcancel are all defined based on data for this bid
topUtil = nlogit_params.c7_cons + nlogit_params.c7_lnnumreps*log(bid.NumReps + 1) + ...
          nlogit_params.c7_buyrep*(bid.SumRep / bid.NumReps) + ...
          nlogit_params.c7_lnprevcancel*log(bid.PreviousCancels + 1);

%% Track down the formulas used here and why they are correct
A = exp(topUtil + nlogit_params.nestCorr*incl_value);
B = exp(utilities(1) - max(utilities));
C = sum(exp( utilities - max(utilities) ));

prob = A/(1+A) * B/C;
%% Where does the formula for the derivative come from here?
prob_deriv = (  prob * nlogit_params.c6_price * (B/(C*(1+A)) + 1/nlogit_params.nestCorr*(1 - B/C)));

end