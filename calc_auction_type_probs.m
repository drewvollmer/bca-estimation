% calc_auction_type_probs.m
% Use an iterative procedure to estimate the probability of each auction being a specific auction
% type.  Auction types are interepreted as the difficulty of the job being procured.  The lowest
% type is for the highest-cost jobs; the highest type is for the lowest.
% The number of possible auction types is the modeler's choice, given in the num_auc_types command
% line argument.
% Call at the command line using:
% matlab -nodisplay -nosplash -r "try, estimate_auction_type_probs([integer > 0]), catch, end, quit"
% Drew Vollmer 2017-12-12


% Frame as a function to use command line arguments
function [] = calc_auction_type_probs(num_auc_types)

% Quality control for the command line parameter
assert(nargin == 1, 'Function only takes one argument.');
assert(num_auc_types == floor(num_auc_types), 'Input parameter must be an integer.');
assert(num_auc_types >= 2, 'Must have two or more auction types (unnecessary for one type).');


%% Part 1: Import data
%% Data format:
%% Auction ID, Bid Type, Auction Observable Type, Bid, [Regressors]
%% AucID, BidType, ObsAucType, BidAmount, [Regressors]

%% TODO: Change to more general format from template data
% Columns:     sscanf(line, "%d %d %d %d %d %d %d %d %d %d %lf %lf %d %lf",
%           &b->bidreqId, &b->decNum,
%           &b->dec, &b->overallDec, &b->sumRep, &b->numReps,
%           &b->buyerAuctions, &b->buyerCancels, &b->numbids,
%           &b->type, &b->avgbid, &b->cost, &b->sellRep, &b->bidAmt);
sampled = readtable(['c:/Users/drewv/Documents/duke_econ/ra_work/bollinger_17_18/' ...
                    'solar_beauty_contest_auctions/yoganarasimhan_simulation_code/sampled-aucs.txt']);


% Name the variables
%% TODO: give some variables better names. Decide what is actually necessary
sampled.Properties.VariableNames = {'AuctionID', 'BidIDinAuction', 'dec', 'overallDec', 'sumRep', ...
                    'numReps', 'buyerAuctions', 'buyerCancels', 'numbids', 'type', 'avgbid', 'cost', ...
                    'sellRep', 'bidAmt'};

% Initialize other variables
% sellRep gives the seller's rating (reputation, from 8 - 10). bidder_type thus gives an ordered ranking of seller
% ratings from 1 to 3
sampled.bidder_type = sampled.sellRep - 7;


% Initialize posterior probability of auction types. The lowest type is for high-cost auctions,
% which are more likely for high average bids in the auction.  Use average bids to make an initial
% guess about auction types.
type_probs = nan(length(sampled.bidder_type), num_auc_types);
% Get quantiles of the bid data
quantiles = linspace(0, 1, num_auc_types + 1); % this includes 0 and 1, which we don't want
avgbid_cutoffs = quantile( sampled.avgbid, quantiles(2:(end - 1)) ); % removes 0 and 1

% Assign .75 of the probability to the range where the average bid falls and spread evenly elsewhere
for i = 1:(num_auc_types - 1)
    % Only need to see if it's higher than the first quantile
    if (i == 1);
        avgbid_in_quantile = (sampled.avgbid >= avgbid_cutoffs(i));
    % For others, need to make sure it falls in the proper range
    else;
        avgbid_in_quantile = ( (sampled.avgbid >= avgbid_cutoffs(i)) & ...
                               (sampled.avgbid < avgbid_cutoffs(i - 1)) );
    end;
    type_probs( avgbid_in_quantile, i ) = .75;
    type_probs( ~avgbid_in_quantile, i ) = .25 / (num_auc_types - 1);
end
% Last column is 1 minus the sum of earlier entries
type_probs(:, end) = 1 - sum(type_probs(:, 1:(end - 1)), 2);



%% Iterate using the iterate_type_probs(sampled) function until auction type probabilities converge.
conv_metric = 1;
iteration_count = 1;
while (conv_metric > 0.000001)

    [conv_metric, updated_type_probs] = iterate_type_probs(sampled, type_probs);
    type_probs = updated_type_probs;
    disp(sprintf('Iterations: %d.  Convergence criterion: %5.8f', iteration_count, conv_metric));
    iteration_count = iteration_count + 1;
end


%% Part 3: use converged probabilities to write inverse CDF

% Get bidder types
bidder_types = unique(sampled.bidder_type);
%% For sample data, remove type -7 (which can probably be removed from actual data)
bidder_types = bidder_types( bidder_types >= 0 );

% Points to include for the density estimation
points = (1:600)';
% Probabilities for inverse CDFs: always take 1000 samples
probabilities = linspace(0, 1, 1000);

% Instantiate a three-dimensional array: one dimension for bidder_types, another for cost types, and the
% last for the number of CDF points to store
inv_cdfs = nan( length(bidder_types), num_auc_types, length(probabilities) );

for bidder_type = bidder_types'

    % Create an index for the bids for this bidder_type
    is_current_bidder_type = (sampled.bidder_type == bidder_type);

    % Get the auction type probabilities for that bidder type
    current_type_probs = type_probs( is_current_bidder_type, :);
    
    % Prices are just given by bidAmt
    prices = sampled.bidAmt(is_current_bidder_type);

    % Estimate and store the inverse CDF
    for auc_type = 1:length(type_probs(1, :))
        [invF, ~] = ksdensity(prices, probabilities, 'support', [min(points) - 1, max(points)], 'width', ...
                                0.1, 'weights', current_type_probs(:, auc_type), 'function', 'icdf');
        inv_cdfs(bidder_type, auc_type, :) = invF;
    end
end

%% Write the inverse CDFs to one file for each bidder type
for bidder_type = bidder_types'
    fid = fopen(sprintf('inv_cdf_%d.csv', bidder_type), 'w');
    fprintf(fid, strcat(repmat('type_%d_prob, ', 1, num_auc_types - 1), ' type_%d_prob\n'), 1:num_auc_types);
    for i = 1:length(inv_cdfs(bidder_type, 1, :))
        fprintf(fid, strcat(repmat('%1.8f, ', 1, num_auc_types - 1), ' %1.8f\n'), inv_cdfs(bidder_type,:, i));
    end
    fclose(fid);    
end

%% Write bid data to a file (if transformation is necessary)

% Entries: AuctionID (BidRequestID), BidIDinAuction (DecisionNo), Decision (dec), overallDec
% (overallDecision), SumRep,  NumReps, buyerAuctions (previousBids), buyerCancels (previousCancels),
% numBids, type (ttype--auction cost), avgbid, cost, sellRep, bidAmount, aucwt, aucwt2

sampled.Properties.VariableNames = {'BidRequestID', 'DecisionNo', 'Decision', 'OverallDecision', 'SumRep', ...
                    'NumReps', 'PreviousBids', 'PreviousCancels', 'numbids', 'ttype', 'avbid', 'cost', ...
                    'sellRep', 'bidAmount', 'bidder_type'};
writetable(sampled, 'synthdata-weights2.txt');



% End statement because the file is framed as a function
end
