% estimate_auction_type_probs.m
% Use an iterative procedure to estimate the probability of each auction being a specific auction
% type.  Auction types are interepreted as the difficulty of the job being procured.
% The number of possible auction types is the modeler's choice, given in the num_auc_types command
% line argument.
% Drew Vollmer 2017-12-12

clear;
clc;

% Higher precision
format long;

cd('c:/Users/drewv/Documents/duke_econ/ra_work/bollinger_17_18/solar_beauty_contest_auctions/rewritten_estimation_code/');

%% Part 1: Import data from sampled-aucs.txt, as in the readLines() function
% Columns:     sscanf(line, "%d %d %d %d %d %d %d %d %d %d %lf %lf %d %lf",
%           &b->bidreqId, &b->decNum,
%           &b->dec, &b->overallDec, &b->sumRep, &b->numReps,
%           &b->buyerAuctions, &b->buyerCancels, &b->numbids,
%           &b->type, &b->avgbid, &b->cost, &b->sellRep, &b->bidAmt);

sampled = readtable(['c:/Users/drewv/Documents/duke_econ/ra_work/bollinger_17_18/' ...
                    'solar_beauty_contest_auctions/yoganarasimhan_simulation_code/sampled-aucs.txt']);

% Name the variables
sampled.Properties.VariableNames = {'AuctionID', 'BidIDinAuction', 'dec', 'overallDec', 'sumRep', ...
                    'numReps', 'buyerAuctions', 'buyerCancels', 'numbids', 'type', 'avgbid', 'cost', ...
                    'sellRep', 'bidAmt'};

% Initialize other variables
% sellRep gives the seller's rating (8, 9, or 10). btype thus gives an ordered ranking of seller
% ratings from 1 to 3
sampled.btype = sampled.sellRep - 7;
% Initial posterior probability that auction has low, medium, or high costs to the seller
sampled.stype = .2*ones(length(sampled.btype), 1);
sampled.stype( sampled.avgbid > 300 ) = .8;
sampled.stype2 = .35*(1 - sampled.stype);
sampled.stype2( sampled.avgbid > 250 ) = .65*( 1 - sampled.stype(sampled.avgbid > 250) );
% stype: probability the auction has a high cost; stype2: probability of a medium cost
% 1 - stype - stype2: probability of a low cost

%% When generalizing to allow arbitrary cost types, it will probably be easier to store weights in a
%% matrix. Then we don't need to refer to them as arbitrarily named, hard-coded columns


%% readLines() completed; data imported


%% Use the iterate_weights(sampled) routine to iterate until convergence, updating weights after
%% each iteration
conv_metric = 1;
iteration_count = 1;
while (conv_metric > 0.000001)

    [conv_metric, updated_stypes] = iterate_type_probs(sampled);
    sampled.stype = updated_stypes(:, 1);
    sampled.stype2 = updated_stypes(:, 2);
    disp(sprintf('Iterations: %d.  Convergence criterion: %5.8f', iteration_count, conv_metric));
    iteration_count = iteration_count + 1;
end

%% PrintEvalReadKde() and UpdateS() completed

%% Last part of sortkde3.cc: write bid data to another(?!) output file and store the CDFs using the
%% converged probabilities

% Calculate CDFs
% Get bidder types (interpreted as reputation; hard-coded for now)
btypes = 1:3;
% Points to include for the density estimation
points = (1:600)';
% Probabilities for inverse CDFs
probabilities = linspace(0, 1, 1000);

% Instantiate a three-dimensional array: one dimension for btypes, another for cost types, and the
% last for the number of CDF points to store
cdfs = nan( length(btypes), 3, length(points) );
inv_cdfs = nan( length(btypes), 3, length(probabilities) );

for btype = btypes

    % Create an index for the bids for this btype
    is_current_btype = (sampled.btype == btype);

    % Get the three different weights for that btype
    weights1 = sampled.stype(is_current_btype);
    weights2 = sampled.stype2(is_current_btype);
    weights3 = 1 - sampled.stype(is_current_btype) - sampled.stype2(is_current_btype);
    
    % Prices are just given by bidAmt
    prices = sampled.bidAmt(is_current_btype);

    % Calculate CDFs
    F_cost1 = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights1, 'function', 'cdf');
    F_cost2 = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights2, 'function', 'cdf');
    F_cost3 = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights3, 'function', 'cdf');
    % And inverse CDFs
    invF1 = ksdensity(prices, probabilities, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights1, 'function', 'icdf');
    invF2 = ksdensity(prices, probabilities, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights2, 'function', 'icdf');
    invF3 = ksdensity(prices, probabilities, 'support', [min(points) - 1, max(points)], 'width', ...
                        0.1, 'weights', weights3, 'function', 'icdf');
    

    % Store in the array
    cdfs(btype, :, :) = [F_cost1, F_cost2, F_cost3]';
    inv_cdfs(btype, :, :) = [invF1; invF2; invF3];
end

%% Write the CDFs to a file (doesn't work nicely since the cdfs are a 3D array)
% Write one file for each type
for btype = btypes
    fid = fopen(sprintf('cdf_%d.csv', btype), 'w');
    fprintf(fid,'stype, stype2, 1 - stype - stype2 \n');
    for i = 1:length(cdfs(btype, 1, :))
        fprintf(fid, '%1.8f, %1.8f, %1.8f', cdfs(btype,:, i));
        fprintf(fid, '\n');
    end
    fclose(fid);
    
    fid = fopen(sprintf('inv_cdf_%d.csv', btype), 'w');
    fprintf(fid,'stype, stype2, 1 - stype - stype2 \n');
    for i = 1:length(inv_cdfs(btype, 1, :))
        fprintf(fid, '%1.8f, %1.8f, %1.8f', inv_cdfs(btype,:, i));
        fprintf(fid, '\n');
    end
    fclose(fid);    
end

%% Write bid data to a file (if transformation is necessary)

% Entries: AuctionID (BidRequestID), BidIDinAuction (DecisionNo), Decision (dec), overallDec
% (overallDecision), SumRep,  NumReps, buyerAuctions (previousBids), buyerCancels (previousCancels),
% numBids, type (ttype--auction cost), avgbid, cost, sellRep, bidAmount, aucwt, aucwt2

sampled.Properties.VariableNames = {'BidRequestID', 'DecisionNo', 'Decision', 'OverallDecision', 'SumRep', ...
                    'NumReps', 'PreviousBids', 'PreviousCancels', 'numbids', 'ttype', 'avbid', 'cost', ...
                    'sellRep', 'bidAmount', 'btype', 'aucwt', 'aucwt2'};
writetable(sampled, 'synthdata-weights2.txt');



%%% Debugging:
% Verified that average S is correct on the first iteration, and densities are being calculated
% properly (output from kde.out matches densities(3, 1, :))
% Logs calculated here and from C++ are different because of rounding. This is presumably the cause
% of the differences in the convergence criterion.