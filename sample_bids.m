% sample_bids.m
% Simulate bids from each bidder type in each type of auction ahead of time.  This is necessary
% because it would be impractical to export the whole joint distribution of bids (X, b) to the C++
% program.  Instead, calculate the joint distribution here and sample 10,000 bids for each type.
% Drew Vollmer 2018-06-28

%% Conclusion: the sample size is highly unlikely to be large enough to estimate a full joint
%% distribution.  We'll need to sample bids in a different way for each dataset, choosing different
%% variables to sample and imposing parametric forms and independence as needed.

clear;
clc;

% Load data

% Import type probabilities to account for unobserved types
type_probs = csvread('unobs_auc_type_probs.csv', 1);
% Import general data for bidder types and observed auction types
data = readtable('template_data.csv');

% Only sample bids that are not the outside option
data = data( data.BidderType > 0, : );

%% Declare data columns for bid trait variables (separated by discrete and continuous)
cont_trait_cols = [4 7];
disc_trait_cols = [2];



%% Calculate density of covariates for each combination of observed auction type, bidder type, and
%% unobserved auction
unique_bidder_types = unique(data.BidderType(data.BidderType > 0, :));
unique_oauc_types = unique(data.OAucType);
unique_uauc_types = (1:length(type_probs(1, :)))';

% Start by getting a matrix containing all permutations of those types
all_types = get_permutations(unique_bidder_types, unique_oauc_types, unique_uauc_types);

% Get a density for every combination of types
for row = 1:length(all_types(:, 1));

    bidder_type = all_types(row, 1);
    oauc_type = all_types(row, 2);
    uauc_type = all_types(row, 3);
    
    % Only look at data for these bidder types in this observed auction type
    current_bids = ((data.BidderType == bidder_type) & (data.OAucType == oauc_type));

    % For continuous bid traits, estimated the PDF at evenly spaced points based on percentiles
    cont_end_percentiles = nan(length(cont_trait_cols), 2);
    for cvar_index = 1:length(cont_trait_cols);
        cvar = cont_trait_cols(cvar_index);
        cont_end_percentiles(cvar_index, :) = quantile(table2array(data(:, cvar)), [.025, .975]);
    end;
    % For discrete bid traits, take the number of unique values
    disc_unique_vals = nan(length(disc_trait_cols), 1);
    for disc_index = 1:length(disc_trait_cols);
        disc_unique_vals(disc_index) = length(table2array(unique(data(:, disc_trait_cols(disc_index)))));
    end;

    % Calculate how many points we can sample for the continuous variables
    round( (sum(current_bids) ./ prod(disc_unique_vals) )^(1./length(cont_trait_cols)) )
    
    
    % The curse of dimensionality is a big deal here.  We might want a way to simplify the joint
    % distribution of bid traits.
    
    % Points to sample for the density
    % Take a linear spacing of the 5th percentile to the 95th for each continuous variable
    % Take discrete values for each discrete variable
    
    % To sample 10,000 points, we can only have (10000)^(1/d) for each dimension.
    % Further, we want the points to represent the distribution of the variable.
    % data(current_bids, bid_trait_cols)
    % sample_points = get_permutations(linspace(sort(unique(data(current_bids, bid_trait_cols(1)))));

    % Calculate the density and weight using the type probabilities for the current uauc_type
    % current_density = mvksdensity(table2array(data(current_bids, bid_trait_cols)), sample_points, ...
    %                               'bandwidth', .5, 'weights', type_probs(current_bids, uauc_type));
end


% Calculate inverse CDF

% Sample 10,000 bids from the inverse CDF using a d-tuple of sampled quantiles

% Write to a series of CSVs