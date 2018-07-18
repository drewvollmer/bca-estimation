% calc_auction_type_probs.m
% Use an iterative procedure to estimate the probability of each auction being a specific unobserved
% auction type.  Auction types are interepreted as the difficulty of the job being procured.  The 
% lowest type is for the highest-cost jobs; the highest type is for the lowest.
% The number of possible auction types is the modeler's choice, given in the NumUAucTypes command
% line argument.
% Call at the command line using:
% matlab -nodisplay -nosplash -r "try, estimate_auction_type_probs([integer > 0]), catch, end, quit"
% Drew Vollmer 2017-12-12


% Frame as a function to use command line arguments
function [] = calc_auction_type_probs(NumUAucTypes)

% Quality control for the command line parameter
assert(nargin == 1, 'Function only takes one argument.');
assert(NumUAucTypes == floor(NumUAucTypes), 'Input parameter must be an integer.');
assert(NumUAucTypes > 0, 'Input parameter must be greater than zero.');


%% Part 1: Import data
%% Data format:
%% Auction ID, Bid Type, Auction Observable Type, Bid, [Regressors]
%% AucID, BidType, ObsAucType, BidAmount, [Regressors]
data = readtable('energysage_data_to_estimate.csv');

% Drop outside option bids
if( any( data.BidderType == 0 ) );
    disp('Removing outside option bids from data...');
    data = data( data.BidderType ~= 0, :);
end;

% Calculate average bid in each auction to determine initial posterior probabilities
% Also label the first bid for each auction, which simplifies computation later on
auction_ids = unique( data.AuctionID );
data.AvgBid = nan(length(data.BidAmount), 1);
data.FirstBidInAuction = zeros(length(data.BidAmount), 1);
for auc = auction_ids'
   isCurrentAuc = ( data.AuctionID == auc );
   data.FirstBidInAuction( find(isCurrentAuc, 1, 'first') ) = 1;
   data.AvgBid(isCurrentAuc) = mean(data.BidAmount( isCurrentAuc ));
end

% Initialize posterior probability of unobserved auction types. The lowest type (high-cost auctions)
% is more likely for high average bids in the auction.  Use average bids to make an initial
% guess about auction types.
if NumUAucTypes > 1;
    type_probs = nan(length(data.BidderType), NumUAucTypes);
    % Get quantiles of the bid data
    quantiles = linspace(0, 1, NumUAucTypes + 1); % this includes 0 and 1, which we don't want
    avgbid_cutoffs = quantile( data.AvgBid, quantiles(2:(end - 1)) ); % removes 0 and 1

    % Assign .75 of the probability to the range where the average bid falls and spread evenly elsewhere
    for i = 1:(NumUAucTypes - 1)
        % Only need to see if it's higher than the first quantile
        if (i == 1);
            avgbid_in_quantile = (data.AvgBid >= avgbid_cutoffs(i));
            % For others, need to make sure it falls in the proper range
        else;
            avgbid_in_quantile = ( (data.AvgBid >= avgbid_cutoffs(i)) & ...
                                   (data.AvgBid < avgbid_cutoffs(i - 1)) );
        end;
        type_probs( avgbid_in_quantile, i ) = .75;
        type_probs( ~avgbid_in_quantile, i ) = .25 / (NumUAucTypes - 1);
    end
    % Last column is 1 minus the sum of earlier entries
    type_probs(:, end) = 1 - sum(type_probs(:, 1:(end - 1)), 2);
else;
    type_probs = ones(length(data.BidderType), NumUAucTypes);
end;


%% Iterate using the iterate_type_probs(data) function until auction type probabilities converge.
conv_metric = 1;
iteration_count = 1;
while (conv_metric > 0.000001)

    [conv_metric, updated_type_probs] = iterate_type_probs(data, type_probs);
    type_probs = updated_type_probs;
    disp(sprintf('Iterations: %d.  Convergence criterion: %5.8f', iteration_count, conv_metric));
    iteration_count = iteration_count + 1;
end

% Write the type probabilities to a CSV (used for simulated auctions)
fid = fopen('unobs_auc_type_probs.csv', 'w');
fprintf(fid, strcat(repmat('type_%d_prob, ', 1, NumUAucTypes - 1), ' type_%d_prob\n'), 1: ...
        NumUAucTypes);
for row = 1:length(type_probs(:, 1));
    fprintf(fid, strcat(repmat('%1.8f, ', 1, NumUAucTypes - 1), ' %1.8f\n'), ...
            type_probs(row, :));
end
fclose(fid);


% Write the probability distribution for the number of bidders for each arrangement of observed types
max_auc_bids = 0;
data.NumBids = zeros(length(data.AuctionID), 1);
for id = unique(data.AuctionID)';
    data.NumBids(data.AuctionID == id) = sum( data.AuctionID == id );
    max_auc_bids = max(max_auc_bids, sum( data.AuctionID == id ));
end;
num_bid_distribution = nan(length(unique(data.OAucType)), max_auc_bids);

for obs_auc_type = (1:length(unique(data.OAucType)));
    for i = 1:max_auc_bids;
            
        num_bid_distribution(obs_auc_type, i) = ...
            length(unique(data.AuctionID(data.NumBids == i & data.OAucType == obs_auc_type))) / ...
            length(unique(data.AuctionID(data.OAucType == obs_auc_type)));
    end;
end;
fid = fopen('num_bid_distribution.csv', 'w');
for row = 1:length(unique(data.OAucType));
    fprintf(fid, strcat(repmat('%6.10f,', 1, max_auc_bids - 1), '%6.10f\n'), ...
            num_bid_distribution(row, :));
end
fclose(fid);



% And write the probability distribution of each bidder type for each arrangement of observed types
bidder_type_dist = nan(length(unique(data.OAucType)), length(unique(data.BidderType)));

for obs_auc_type = (1:length(unique(data.OAucType)));
    for i = 1:length(unique(data.BidderType));
        
        bidder_type_dist(obs_auc_type, i) = sum(data.BidderType(data.OAucType == obs_auc_type) == i) ...
            / length(data.BidderType(data.OAucType == obs_auc_type));    
    end;
end;
fid = fopen('bidder_type_distribution.csv', 'w');
for row = 1:length(unique(data.OAucType));
    fprintf(fid, strcat(repmat('%6.10f,', 1, length(unique(data.BidderType)) - 1), '%6.10f\n'), ...
            bidder_type_dist(row, :));
end
fclose(fid);






% %% Part 3: use converged probabilities to write inverse CDF

% % Get a unique set of bidder types
% bidder_types = unique(data.BidderType);
% % This should only include positive types: outside option (type 0) has been removed
% assert( all(bidder_types > 0), 'Bidder types must be positive.');

% % Get a unique set of observed auction types
% obs_auc_types = unique(data.OAucType);

% % Points to include for the density estimation
% points = linspace(max(1, min(data.BidAmount) - 10), ...
%                   1.05*max(data.BidAmount), 1000)';
% % Probabilities for inverse CDFs: always take 1000 samples
% probabilities = linspace(0, 1, 1000);

% % Instantiate a four-dimensional array: need the bid distribution as a function of bid traits,
% % observed auction traits, unobserved auction traits, and the points at which we evaluate
% inv_cdfs = nan( length(bidder_types), length(obs_auc_types), NumUAucTypes, length(probabilities) );

% for bidder_type = bidder_types'

%     % Create an index for the bids for this bidder_type
%     is_current_bidder_type = (data.BidderType == bidder_type);
    
%     for obs_auc_type = obs_auc_types'

%         % Current sample: only take observations for the current bidder and observed auction type
%         current_sample = ( is_current_bidder_type & (data.OAucType == obs_auc_type) );

%         % Get unobserved type probabilities and prices for the sample
%         current_type_probs = type_probs( current_sample, :);
%         prices = data.BidAmount(current_sample);

%         % Estimate and store the inverse CDF
%         for unobs_auc_type = 1:length(type_probs(1, :))
%             [invF, ~] = ksdensity(prices, probabilities, 'support', [min(points) - 1, max(points)], 'width', ...
%                                   0.1, 'weights', current_type_probs(:, unobs_auc_type), 'function', 'icdf');
%             inv_cdfs(bidder_type, obs_auc_type, unobs_auc_type, :) = invF;
%         end
        
%     end
    
% end

% %% Write the inverse CDFs to one file for each bidder type/observed auction type pair
% for bidder_type = bidder_types'
%     for obs_auc_type = obs_auc_types'
%         fid = fopen(sprintf('inv_cdf_bid%d_obsat%d.csv', bidder_type, obs_auc_type), 'w');
%         fprintf(fid, strcat(repmat('type_%d_prob, ', 1, NumUAucTypes - 1), ' type_%d_prob\n'), 1:NumUAucTypes);
%         for i = 1:length(inv_cdfs(bidder_type, obs_auc_type, 1, :))
%             fprintf(fid, strcat(repmat('%1.8f, ', 1, NumUAucTypes - 1), ' %1.8f\n'), ...
%                     inv_cdfs(bidder_type, obs_auc_type, :, i));
%         end
%         fclose(fid);
%     end
% end


% %% Write bid data to a file (if transformation is necessary)

% % Entries: AuctionID (BidRequestID), BidIDinAuction (DecisionNo), Decision (dec), overallDec
% % (overallDecision), SumRep,  NumReps, buyerAuctions (previousBids), buyerCancels (previousCancels),
% % numBids, type (ttype--auction cost), avgbid, cost, sellRep, bidAmount, aucwt, aucwt2

% data.Properties.VariableNames = {'BidRequestID', 'DecisionNo', 'Decision', 'OverallDecision', 'SumRep', ...
%                     'NumReps', 'PreviousBids', 'PreviousCancels', 'numbids', 'ttype', 'avbid', 'cost', ...
%                     'sellRep', 'bidAmount', 'bidder_type'};
% writetable(data, 'synthdata-weights2.txt');



% End statement because the file is framed as a function
end
