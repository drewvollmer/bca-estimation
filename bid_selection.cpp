// bid_selection.cpp
// Implementing the functions and classes declared in bid_selection.hpp
// This is the file to edit when changing the bid selection model
// Drew Vollmer 2018-01-17

// Import the header file whose declarations we want to implement
#include "bid_selection.hpp"


using namespace std;


// Data types are defined in bid_selection.hpp


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


// Implement function to import bid selection parameters
BidSelectionParams getBidSelectionParams(){

    ifstream infile("coeff.txt");
    string line;
    // Ignore the first (header) row
    getline(infile, line);
    int numCols = count(line.begin(), line.end(), '\t') + 1;

    // Read the second row (containing the coefficients) into a vector
    string readParam;
    vector<double> nlogitParams;

    for(int i = 0; i < numCols; i++){
        infile >> readParam;
        // Ignore the first entry, y1 (string.compare() returns 0 for a match)
        if( readParam.compare("y1") == 0 ){
            continue;
        }
        // Use atof() and .c_str() to convert string to double and append
        nlogitParams.push_back( atof(readParam.c_str()) );
    }

    // Unpack the vector into the parameter object
    BidSelectionParams nestedLogitParams;
    nestedLogitParams.bidAmountCoeff = nlogitParams[0];
    nestedLogitParams.sellRepCoeff = nlogitParams[1];
    nestedLogitParams.nestConstant = nlogitParams[5];
    nestedLogitParams.lnnumrepsCoeff = nlogitParams[2];
    nestedLogitParams.buyrepCoeff = nlogitParams[3];
    nestedLogitParams.lnprevcancelCoeff = nlogitParams[4];
    nestedLogitParams.nestCorr = nlogitParams[7];

    // Debugging: read out values
    // cout << "bidAmount: " << nestedLogitParams.bidAmountCoeff << "\n";
    // cout << "sellRep: " << nestedLogitParams.sellRepCoeff << "\n";
    // cout << "nestConstant: " << nestedLogitParams.nestConstant << "\n";
    // cout << "lnnumreps: " << nestedLogitParams.lnnumrepsCoeff << "\n";
    // cout << "buyrep: " << nestedLogitParams.buyrepCoeff << "\n";
    // cout << "lnprevcancel: " << nestedLogitParams.lnprevcancelCoeff << "\n";
    // cout << "nestCorr: " << nestedLogitParams.nestCorr << "\n";
    
    return( nestedLogitParams );
}



// Implement function to simulate an auction
pair<double, double> simulateAuction(Bid currentBid, int uAucType, int numBidderTypes,
                                     vector< vector< vector< vector<Bid> > > >& sampleBids,
                                     BidSelectionParams& bidSelParams){

    int numOtherBids = 9;
    
    // cout << "Starting bid: " << currentBid.amount << "; type " << currentBid.bidderType << "\n";
    
    // TODO: distribution of other bidders
    
    cout << "Starting bid: " << currentBid.amount << "; type " << currentBid.bidderType << ", " << currentBid.state[0] << "\n";

    // TODO: distribution of bidder types in this observed type of auction
    // For now, randomly assign other bidder types with draws from {0, 1, ..., numBidderTypes - 1}

    // Draw numOtherBids random bids from the sample for this observed auction type and the relevant bidder type
    // Each draw is from {0, ..., 9999}
    int bidTypes[numOtherBids + 1];
    Bid bids[numOtherBids + 1];
    bidTypes[0] = currentBid.bidderType - 1; // Subtract 1 to adjust for indexing that starts at 0
    bids[0] = currentBid;
    for(int i = 1; i < numOtherBids + 1; i++){
        bidTypes[i] = random() % numBidderTypes;
        bids[i] = sampleBids[bidTypes[i]][currentBid.obsAucType - 1][uAucType][random() % 10000];

        // Fill in the auction-specific parts of the bid (memcpy needed to assign the array)
        memcpy(bids[i].state, currentBid.state, sizeof(currentBid.state));

        // cout << uAucType << "; " << bidTypes[i] << "; " << bids[i] << "\n";
    }


    // DEBUGGING: set bids and bid types
    // double bids[numOtherBids + 1] = {321, 246.528, 218.8331, 261.0508, 250.574, 257.6782, 200.9693, 264.1822, 261.9003, 185.5633};
    // int bidTypes[numOtherBids + 1] = {2, 2, 0, 2, 0, 1, 1, 2, 2, 0};

    
    // Use simulated auction for utility calculations to get probability and its derivative
    // Calculate the utility of each bid using the nested logit parameters
    double utilities[numOtherBids + 1];
    for(int i = 0; i < numOtherBids + 1; i++){
        utilities[i] = (bidSelParams.bidAmountCoeff*bids[i] + bidSelParams.sellRepCoeff*bidTypes[i])
            / bidSelParams.nestCorr;
        // Entry 1: c6_price; entry 2: c6_sellrep; entry 8: nestCorr        
    }


    // Account for the fact that numReps might be zero
    double buyRepVal = (currentBid.numReps > 0 ? (currentBid.sumRep / currentBid.numReps) : 0);
    double nestUtil = bidSelParams.nestConstant + bidSelParams.lnnumrepsCoeff*log(currentBid.numReps + 1) +
        bidSelParams.buyrepCoeff*buyRepVal + bidSelParams.lnprevcancelCoeff*log(currentBid.previousCancels + 1);
    // Entry 6: c7_cons; entry 3: c7_lnnumreps; entry 4: c7_buyrep; entry 5: lnprevcancel

    // Traverse the array to add exp(utility) and find the max
    double expUtilSum = exp(utilities[0]);
    for(int i = 1; i < numOtherBids + 1; i++){
        expUtilSum += exp( utilities[i] );
    }
    double incVal = log( expUtilSum );

    // cout << "nestUtil: " << nestUtil << "; incVal: " << incVal << "; expUtilSum: " << expUtilSum << "\n";
    // cout << "utilities[0]: " << utilities[0] << "\n";
    
    double A = exp(nestUtil + bidSelParams.nestCorr*incVal);
    double B = exp(utilities[0]);
    double C = expUtilSum;

    // printf("A: %lf. B: %lf. C: %lf.\n", A, B, C);

    // First entry is selection probability; second is its derivative
    pair<double, double> auctionResult;
    auctionResult.first = A/(1+A) * B/C;
    auctionResult.second = (  auctionResult.first * bidSelParams.bidAmountCoeff * (B/(C*(1+A)) + 1/bidSelParams.nestCorr*(1 - B/C)) );
    
    // printf("prob: %lf. probDer: %lf.\n", auctionResult.first, auctionResult.second);
    return( auctionResult );

}
