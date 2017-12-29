% simulate3-clean.m
% Replicating the strategy and output of simulate3-clean.cc
% Drew Vollmer 2017-12-20

clear;
clc;

cd('c:/Users/drewv/Documents/duke_econ/ra_work/bollinger_17_18/solar_beauty_contest_auctions/rewritten_estimation_code/');

%% Read in CDFs and parameters from sortkde3.m and synthdata.do
inv_cdf_files = dir('inv_cdf_*.csv');
for i = 1:length(inv_cdf_files);

    file = inv_cdf_files(i);
    
    % On the first iteration of the loop, get parameters needed for the CDF array
    if i == 1;
        first_inv_cdf = csvread(file.name, 1, 0); % Ignore column names
        inv_cdfs = nan( length(inv_cdf_files), length(first_inv_cdf(1, :)), length(first_inv_cdf(:, 1)) );
        clear first_inv_cdf;
    end;

    inv_cdfs(i, :, :) = csvread(file.name, 1, 0)';
end

% Throws a warning for variable name formatting
nlogit_params = readtable('coeff.txt');
%%%% Check to make sure this agrees with the C++ reading
nlogit_params.Properties.VariableNames = {'ign1', 'c6_price', 'c6_sellrep', 'c7_lnnumreps', 'c7_buyrep', ...
                    'c7_lnprevcancel', 'c7_cons', 'ign2', 'nestCorr', 'unknown'};

%% Read each line from synthdata-weights2.txt (sortkde3 output)
auctions = readtable('synthdata-weights2.txt');

%% Simulate the probability of a bid winning and the derivative of the probability for each auction
%% type. Do so by simulating [num_sims] auctions for each bid and storing the means
% Initialize array to store probabilities: entries for each bid and auction type
bid_probs = nan( length(auctions.bidAmount), 3, 2);
% Declare number of auction simulations
num_sims = 1000;

%% This takes 21 seconds for the first auction alone.  Need to speed it up somehow, perhaps with
%% compiled code. Changing from a function offers no benefit.
tic
%for i = 1:length(auctions.bidAmount)
for i = 1:11
    
    % If this is the "header" bid with no bid amount, continue
    %% (This is a quirk of the sample data.)
    if auctions.bidAmount(i) == 0;
        continue;
    end;

    % Otherwise, simulate the auction 1,000 times for each auction cost
    for auc_cost = 1:3
            
        prob_matrix = nan(num_sims, 2);
        for j = 1:num_sims;
            %prob_matrix(j, :) = simulate_auction(auctions(i,:), auc_cost, nlogit_params, inv_cdfs);
        end;
        % Store the average of the probability and its derivative
        bid_probs(i, auc_cost, :) = sum(prob_matrix) / num_sims;
    
    end
end
toc

% Calculate costs
costs = .85*(auctions.bidAmount + bid_probs(:, :, 1)./bid_probs(:, :, 2));
