% iterate_type_probs.m
% A helper function iterating the density estimation and probability distribution of the project
% cost for each auction.  Returns the updated probabilities and a criterion for convergence.
% Drew Vollmer 2017-12-13

function [conv_metric, updated_stypes] = iterate_type_probs(auctions)

%% To fix: bidder types, cost types, and density points are hard-coded.
    
% Get bidder types (interpreted as reputation; hard-coded for now)
btypes = 1:3;
% Points to include for the density estimation
points = (1:600)';

% Instantiate a three-dimensional array: one dimension for btypes, another for cost types, and the
% last for the number of density points to store
densities = nan( length(btypes), 3, length(points) );

% Define a lower bound for the probability in each density
low_prob = 0.000001;

for btype = btypes

    % Create an index for the bids for this btype
    is_current_btype = (auctions.btype == btype);

    % Get the three different weights for that btype
    weights1 = auctions.stype(is_current_btype);
    weights2 = auctions.stype2(is_current_btype);
    weights3 = 1 - auctions.stype(is_current_btype) - auctions.stype2(is_current_btype);
    
    % Prices are just given by bidAmt
    prices = auctions.bidAmt(is_current_btype);

    % Calculate densities at the relevant points
    [f_cost1, ~] = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', 0.1, 'weights', weights1);
    [f_cost2, ~] = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', 0.1, 'weights', weights2);
    [f_cost3, ~] = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], 'width', 0.1, 'weights', weights3);

    % Reassing values below the arbitrary low probability to be a low but finite probability
    % (necessary because the iteration uses logs)
    f_cost1( f_cost1 < low_prob ) = low_prob;
    f_cost2( f_cost2 < low_prob ) = low_prob;
    f_cost3( f_cost3 < low_prob ) = low_prob;
    
    % Store in the array: cost3 corresponds to s = 0 in original array
    densities(btype, :, :) = [f_cost1, f_cost2, f_cost3]';    
end
% Remove now unnecessary objects
clear weights1 weights2 weights3 is_current_btype f_cost1 f_cost2 f_cost3;


%% UpdateS()

% Averages of each stype are used to update weights
avg_stype = sum( auctions.stype(auctions.BidIDinAuction == 1) ) / sum(auctions.BidIDinAuction == 1);
avg_stype2 = sum( auctions.stype2(auctions.BidIDinAuction == 1) ) / sum(auctions.BidIDinAuction == 1);

% Report as in the original code
%sprintf('S: %5.5f %5.5f', avg_stype, avg_stype2)

% A quirk of the randomly sampled data is that auction IDs could be repeated. Replace with a
% standard AuctionID
auctions.AuctionID = -1*ones(length(auctions.AuctionID), 1);
i = 1;
id = 1;
while (i < length(auctions.AuctionID))
    auctions.AuctionID( i : (i + auctions.numbids(i)) ) = id;
    id = id + 1;
    i = i + auctions.numbids(i) + 1;    
end
clear i id;




% Prepare to loop over all auctions. Get a vector of all IDs
auction_ids = unique( auctions.AuctionID );

% Initialize a variable to measure the total difference in stypes
tot_diff = 0;
% Initialize a matrix of updated stypes to be returned
updated_stypes = nan(length(auctions.stype), 2);


% %% Debugging: write and read densities so that they're stored at the precision used by C++
% %% This means 5 significant digits.  The differences for very small numbers are producing large
% %% differences in the convergence criteria
% for i = 1:3;
%     for j = 1:3;
%         dlmwrite('kde_densities.csv', densities(i, j, :));
%         densities(i, j, :) = csvread('kde_densities.csv');
%     end;
% end;

for auc = auction_ids'

    % Get all bids in this auction (stored as a table)
    auc_index = (auctions.AuctionID == auc);
    auc_bids = auctions( auc_index, :);


    % Add the density corresponding to each bid and cost type:
    % Need to access the btype for each bid and add the density at the bid amount for that btype to
    % the sums
    % btype vector: auc_bids.btype; amount vector: auc_bids.bidAmt
    %sub2ind(size(densities), auc_bids.btype, :, auc_bids.bidAmt);
    % Loop excludes the first bid, which here has no true btype
    auction_density = nan( length(auc_bids.btype) - 1, 3 ); % subtract 1 since the first entry is
                                                            % not used
    %% Note: need bidAmt(bid) + 1 to match original code.  Not clear if this is correct.  It depends
    %% whether the density on [0, 600] excludes 0 or 600.
    for bid = 2:length(auc_bids.AuctionID)
        auction_density(bid - 1, :) = densities(auc_bids.btype(bid), :, auc_bids.bidAmt(bid) + 1 ); % subtract 1 for btype
    end
    % Define a vector f (following original programs) to store log all sums
    f = log( [avg_stype, avg_stype2, (1 - avg_stype - avg_stype2)] ) + sum(log(auction_density));
    
    % For each auction:
    % 1. Get the btype of each bid and sum the log density of the observed bid amount of that btype for each cost type
    % (i.e. log likelihood)
    % 2. Add avg_stype, avg_stype2, or 1 - avg_stype - avg_stype2 to the corresponding sum
    % Done: f initialized for step 2, then the loop takes care of step 1
    
    % 3. Take the max of the sums
    fmax = max(f);
    % 5. Update stype and stype2 as exp(sum1 - max_sum) / \Sigma_i exp(sumi - max_sum)
    old_stypes = [auc_bids.stype, auc_bids.stype2];
    new_stypes = [ (exp(f(1) - fmax)/(exp(f(1) - fmax) + exp(f(2) - fmax) + exp(f(3) - fmax))), ...
                   (exp(f(2) - fmax)/(exp(f(1) - fmax) + exp(f(2) - fmax) + exp(f(3) - fmax))) ];
    % 6. Calculate the difference between old and new stype and stype2 and add to a cumulative sum
    % (which will be updated over all auctions)
    tot_diff = tot_diff + sum( abs( old_stypes(1,:) - new_stypes ) );
    
    % Store new_stypes in the updated_stypes matrix to be returned
    updated_stypes(auc_index, :) = repmat(new_stypes, length(auc_bids.AuctionID), 1);
end

% 7. Return the cumulative difference over 2*num_auctions as the convergence standard
conv_metric = tot_diff / (2*length(auction_ids));

end