// bid_selection.cpp
// Implementing the functions and classes declared in bid_selection.hpp
// This is the file to edit when changing the bid selection model
// Drew Vollmer 2018-01-17

// Import the header file whose declarations we want to implement
#include "bid_selection.hpp"


using namespace std;

// Implement function to import bid data
// Get bid data for the next bid in the file. Works by taking in an object referring to the file, then
// extracting the next line (bid) and processing it.  Returns a Bid object.
Bid getBidData(FILE *bidFile){

    char line[10000];
    char *res = fgets(line, sizeof(line), bidFile);

    // Declare bid object to return and fill with the current line
    Bid currentBid;
    currentBid.isLastBid = !res;
    // Also declare placeholder variables to be read into that we don't care about
    int intToIgnore[4];

    // Order and usage of data:
    // AuctionID (ignored), BidderType (ignored), ObservedAucType, BidAmount (used),
    // Decision (ignored), OverallDecision (ignored), [Regressors] (used)
    sscanf(line, "%d, %d, %d, %lf, %d, %d, %d, %d, %d, %d, %d",
           &intToIgnore[0], &currentBid.bidderType, &currentBid.obsAucType, &currentBid.amount,
           &intToIgnore[1], &intToIgnore[2], &currentBid.sumRep, &currentBid.numReps,
           &currentBid.previousAuctions, &currentBid.previousCancels, &intToIgnore[3]);
    // cout << currentBid.amount << "; " << currentBid.sumRep << "; " << currentBid.numReps << "; " << currentBid.previousAuctions << "; " << currentBid.previousCancels << "; " << currentBid.bidderType << "\n";

    return( currentBid );

}



// Implement function to simulate an auction
pair<double, double> simulateAuction(Bid currentBid, int uAucType, int numBidderTypes,
                                     vector< vector< vector< vector<double> > > >& invCDFs,
                                     vector<double>& nlogitParams){

    int numOtherBids = 9;
    
    // cout << "Starting bid: " << currentBid.amount << "; type " << currentBid.bidderType << "\n";
    
    // Draw numOtherBids integers in {0, 1, ..., numBidderTypes} for bidder types. Then draw the same number of quantiles
    // in {0, ..., 999} and get the bid corresponding to the quantile and bidder type from the inverse CDF
    int bidTypes[numOtherBids + 1];
    double bids[numOtherBids + 1];
    bidTypes[0] = currentBid.bidderType - 1; // Subtract 1 to adjust for indexing that starts at 0
    bids[0] = currentBid.amount;
    for(int i = 1; i < numOtherBids + 1; i++){
        bidTypes[i] = random() % numBidderTypes;
        bids[i] = invCDFs[bidTypes[i]][currentBid.obsAucType - 1][uAucType][random() % 1000];
        // cout << uAucType << "; " << bidTypes[i] << "; " << bids[i] << "\n";
    }


    // DEBUGGING: set bids and bid types
    // double bids[numOtherBids + 1] = {321, 246.528, 218.8331, 261.0508, 250.574, 257.6782, 200.9693, 264.1822, 261.9003, 185.5633};
    // int bidTypes[numOtherBids + 1] = {2, 2, 0, 2, 0, 1, 1, 2, 2, 0};

    
    // Use simulated auction for utility calculations to get probability and its derivative
    // Calculate the utility of each bid using the nested logit parameters
    double utilities[numOtherBids + 1];
    for(int i = 0; i < numOtherBids + 1; i++){
        utilities[i] = (nlogitParams[1]*bids[i] + nlogitParams[2]*bidTypes[i]) / nlogitParams[8];
        // Entry 1: c6_price; entry 2: c6_sellrep; entry 8: nestCorr        
    }

    // cout << "c6 price: " << nlogitParams[1] << "; " << "c6 sellrep: " << nlogitParams[2] << "; " << "nestCorr: "
    //      << nlogitParams[8] << "\n";

    // Account for the fact that numReps might be zero
    double buyRepVal = (currentBid.numReps > 0 ? (currentBid.sumRep / currentBid.numReps) : 0);
    double nestUtil = nlogitParams[6] + nlogitParams[3]*log(currentBid.numReps + 1) +
        nlogitParams[4]*buyRepVal + nlogitParams[5]*log(currentBid.previousCancels + 1);
    // Entry 6: c7_cons; entry 3: c7_lnnumreps; entry 4: c7_buyrep; entry 5: lnprevcancel

    // Traverse the array to add exp(utility) and find the max
    double expUtilSum = exp(utilities[0]);
    for(int i = 1; i < numOtherBids + 1; i++){
        expUtilSum += exp( utilities[i] );
    }
    double incVal = log( expUtilSum );

    // cout << "nestUtil: " << nestUtil << "; incVal: " << incVal << "; expUtilSum: " << expUtilSum << "\n";
    // cout << "utilities[0]: " << utilities[0] << "\n";
    
    double A = exp(nestUtil + nlogitParams[8]*incVal);
    double B = exp(utilities[0]);
    double C = expUtilSum;

    // printf("A: %lf. B: %lf. C: %lf.\n", A, B, C);

    // First entry is selection probability; second is its derivative
    pair<double, double> auctionResult;
    auctionResult.first = A/(1+A) * B/C;
    auctionResult.second = (  auctionResult.first * nlogitParams[1] * (B/(C*(1+A)) + 1/nlogitParams[8]*(1 - B/C)) );
    
    // printf("prob: %lf. probDer: %lf.\n", auctionResult.first, auctionResult.second);
    return( auctionResult );

}
