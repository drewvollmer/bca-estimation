% iterate_type_probs.m
% A helper function iterating the density estimation and probability distribution of the project
% cost for each auction.  Returns the updated probabilities and a criterion for convergence.
% Drew Vollmer 2017-12-13

function [conv_metric, updated_type_probs] = iterate_type_probs(auctions, type_probs)

% Get a unique set of bidder types
bidder_types = unique(auctions.BidderType);
% Remove negative types (used for outside option data)
bidder_types = bidder_types( bidder_types >= 0 );

% Get a unique set of observed auction types
obs_auc_types = unique(auctions.OAucType);


%%% Part 1: Estimate kernel densities as a function of bidder types, observed auction types,
%%% and unobserved auction types

% Points to include for the density estimation (exclude outside option from min and max)
points = linspace(min(auctions.BidAmount(auctions.BidAmount >= 0)), 1.05*max(auctions.BidAmount))';

% Instantiate a four-dimensional array: need the bid distribution as a function of bid traits,
% observed auction traits, unobserved auction traits, and the points at which we evaluate
densities = nan( length(bidder_types), length(obs_auc_types), length(type_probs(1, :)), ...
                 length(points) );

% Define a lower bound for the probability in each density
low_prob = 0.000001;

for bidder_type = bidder_types'

    % Create an index for the bids for this bidder_type
    is_current_bidder_type = (auctions.BidderType == bidder_type);
    
    for obs_auc_type = obs_auc_types'
        
        % Current sample: only take observations for the current bidder and observed auction type
        current_sample = ( is_current_bidder_type & (auctions.OAucType == obs_auc_type) );
        
        % Get unobserved type probabilities and prices for the sample
        current_type_probs = type_probs( current_sample, :);
        prices = auctions.BidAmount(current_sample);
        
        % Run a kernel density to get the distribution of bids for this sample for each unobserved
        % auction type, and store
        for unobs_auc_type = 1:length(type_probs(1, :))
            [f_cost, ~] = ksdensity(prices, points, 'support', [min(points) - 1, max(points)], ...
                                    'width', 0.1, 'weights', current_type_probs(:, unobs_auc_type));
            f_cost( f_cost < low_prob ) = low_prob;
            densities(bidder_type, obs_auc_type, unobs_auc_type, :) = f_cost;
        end    
        
    end

end
% Remove now unnecessary objects
clear is_current_bidder_type f_cost current_type_probs prices


%%% Part 2: Update type probabilities using kernel density results

% Averages of each type probability are used to update the weights
avg_type_probs = sum( type_probs(auctions.FirstBidInAuction == 1, :) ) / ...
                 sum(auctions.FirstBidInAuction == 1);



% Initialize a variable to measure the total difference in type probabilities
tot_diff = 0;
% Initialize a matrix of updated type probabilities to be returned
updated_type_probs = nan(size(type_probs));

% Loop over all auctions
auction_ids = unique( auctions.AuctionID );
for auc = auction_ids'

    % Get all bids in this auction (stored as a table)
    auc_index = (auctions.AuctionID == auc);
    auc_bids = auctions( auc_index, :);
    % And find which bids aren't for the outside option
    inside_bids = find( auc_bids.BidAmount > 0 );

    % Get the density for each inside option bid for each type of auction and add them together
    auction_density = nan( length(inside_bids), length(type_probs(1, :)) );
    row = 1;
    for bid = inside_bids'
        % Get the closest entry in the density
        density_index = max(round( length(points) .* (auc_bids.BidAmount(bid) - min(points)) ./ ...
                                   (max(points) - min(points)) ), 1);
        auction_density(row, :) = densities(auc_bids.BidderType(bid), auc_bids.OAucType(bid), :, density_index );
        row = row + 1;
    end
    % Define a vector f (following original programs) to store log all sums
    f = log( avg_type_probs ) + sum(log(auction_density));
    
    % For each auction:
    % 1. Get the bidder_type of each bid and sum the log density of the observed bid amount of that bidder_type for each cost type
    % (i.e. log likelihood)
    % 2. Add avg_stype, avg_stype2, or 1 - avg_stype - avg_stype2 to the corresponding sum
    % Done: f initialized for step 2, then the loop takes care of step 1
    
    % 3. Update auction type probabilities as exp(sum_1 - max_sum) / \Sigma_i exp(sum_i - max_sum)
    new_type_probs = exp(f - max(f)) ./ sum( exp(f - max(f)) );
    
    % 4. Calculate the difference between original and new auction type probabilities (to be
    % aggregated over all auctions)
    tot_diff = tot_diff + sum( abs( unique(type_probs(auc_index, :), 'rows') - new_type_probs ) );
    
    % Store new_stypes in the updated_stypes matrix to be returned
    updated_type_probs(auc_index, :) = repmat(new_type_probs, length(auc_bids.AuctionID), 1);
end

% 7. Return the cumulative difference over 2*num_auctions as the convergence standard
conv_metric = tot_diff / (2*length(auction_ids));

end