// calculate_costs.cc
// Simulate auctions for each bid; use the resulting choice probabilities to infer seller costs
// Drew Vollmer 2017-12-22

// Import libraries for functions and data structures
#include <stdio.h>
#include <iostream> // for cout
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <cstring>
#include <fstream> // for infile()
#include <algorithm> // to count character occurrences in string: std::count() (also for some string methods)
#include <vector> // For vector class

// Use standard namespace: can call cout and vector rather than std::cout and std::vector
using namespace std;




////////////////////////////////////////////////////////////
//// Class definitions

// Bid class to store important bid data
typedef struct {
    double amount;
    int sellRep;
    int sumRep;
    int numReps;
    int buyerAuctions;
    int previousCancels;
    bool isLastBid; // Tells if this bid is the last one in the file
} Bid;


// Auction traits class storing the number of types used in the auction
// (Inferred from data files in getAucTraits() at runtime.)
typedef struct {
    int numBidderTypes;
    int numAucTypes;
} AucTraits;



///////////////////////////////////////////////////////////////////////////////////
//// Functions

//// Functions to import data

// Get number of bidder types and auction types in the data
AucTraits getAucTraits(){

    // Declare variable to return
    AucTraits aucTraits;

    // Assuming CDFs are in files of the form "inv_cdf_[0-9].csv", one for each type of bidder, find the number of bidders
    // and thus the number of files to import
    int numFiles = 0;
    char fileName[50];
    size_t numCols;

    // Strategy: count the files that exist and break after the first file that does not exist
    while(true){
        sprintf(fileName, "inv_cdf_%d.csv", numFiles + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numFiles++;

            // Use the first file to get the number of columns in the data
            if( numFiles == 1 ){
                string firstLine;
                getline(infile, firstLine);
                aucTraits.numAucTypes = count(firstLine.begin(), firstLine.end(), ',') + 1;
            }
                
        } else {
            break;
        }
    }

    aucTraits.numBidderTypes = numFiles;
    return( aucTraits );
}


// Import inverse CDFs
// Note that this function returns void, not the invCDFs 3D vector because of its size.  Instead,
// the function returns void and fills in the vector using a reference to its memory address.
void importInverseCDFs(vector< vector< vector<double> > >& invCDFs, AucTraits aucTraits){

    // Loop over all files; start by initializing variables used in the loops
    int lineNum;
    char fileName[100];
    string stringToRead;
    string line;
    
    for(int i = 0; i < aucTraits.numBidderTypes; i++){

        // Get file name using current name index
        sprintf(fileName, "inv_cdf_%d.csv", i + 1);

        // Use ifstream to handle the unknown (at compile time) number of columns
        ifstream infile(fileName);
        // Ignore the first (header) row
        getline(infile, line);
        
        // Use a double-loop over known dimensions to fill in invCDFs[i]
        for(int row = 0; row < 1000; row++){
            for( int currentCol = 0; currentCol < aucTraits.numAucTypes; currentCol++){

                // Read the [i][currentCol][row] entry from the file
                infile >> stringToRead;

                // Remove trailing commas, if there are any
                stringToRead.erase( remove(stringToRead.begin(), stringToRead.end(), ','), stringToRead.end() );
                // Convert to double and insert into invCDFs vector
                invCDFs[i][currentCol][row] = atof( stringToRead.c_str() );
            }
        }
        // Finished processing the current file; next loop iteration handles the next file
    }

}

// Import nested logit parameters
vector<double> importNLogitParams(){

    ifstream infile("coeff.txt");
    string line;
    // Ignore the first (header) row
    getline(infile, line);
    int numCols = count(line.begin(), line.end(), '\t') + 1;

    string toInsert;
    vector<double> nlogitParams(numCols, 0);

    for(int i = 0; i < numCols; i++){
        infile >> toInsert;
        // Ignore the first entry, y1 (string.compare() returns 0 for a match)
        if( toInsert.compare("y1") == 0 ){
            continue;
        }
        // Use atof and .c_str() to convert string to double
        nlogitParams[i] = atof(toInsert.c_str());
    }

    return( nlogitParams );
}

// Get bid data for the next bid in the file. Works by taking in an object referring to the file, then
// extracting the next line (bid) and processing it.  Returns a Bid object.
Bid getBidData(FILE *bidFile){

    char line[10000];
    char *res = fgets(line, sizeof(line), bidFile);

    // Declare bid object to return and fill with the current line
    Bid currentBid;
    currentBid.isLastBid = !res;
    // Also declare placeholder variables to be read into that we don't care about
    int intToIgnore[11];
    double doubleToIgnore[4];

    // Line structure: BidRequestID, DecisionNum, Decision, OverallDecision, SumRep, NumReps, PreviousBids,
    // PreviousCancels, NumBids, TrueType, AvgBid, Cost, SellRep, BidAmount, AucWt, AucWt2
    sscanf(line, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %d, %lf, %d, %lf, %lf",
           &intToIgnore[0], &intToIgnore[1], &intToIgnore[2], &intToIgnore[3], &currentBid.sumRep, &currentBid.numReps,
           &currentBid.buyerAuctions, &currentBid.previousCancels, &intToIgnore[8], &intToIgnore[9], &doubleToIgnore[0],
           &doubleToIgnore[1], &intToIgnore[10], &currentBid.amount, &currentBid.sellRep, &doubleToIgnore[2], &doubleToIgnore[3]);

    return( currentBid );

}

///////////////////////////////////////////////////////////////////////////////////
//// Analysis functions: simulate the result of an auction for an input bid

// simulateAuction returns the probability that the input bid is selected in a simulated
// auction and the derivative of the probability.  Both are used to calculate costs.
pair<double, double> simulateAuction(Bid currentBid, int aucType, int numBidderTypes, vector< vector< vector<double> > >& invCDFs,
                                     vector<double>& nlogitParams){

    int numOtherBids = 9;

    //cout << "Starting bid: " << currentBid.amount << "; type " << currentBid.sellRep << "\n";
    
    // Draw numOtherBids integers in {0, 1, ..., numBidderTypes} for bidder types. Then draw the same number of quantiles
    // in {0, ..., 999} and get the bid corresponding to the quantile and bidder type from the inverse CDF
    int bidTypes[numOtherBids + 1];
    double bids[numOtherBids + 1];
    bidTypes[0] = currentBid.sellRep - 1; // Subtract 1 to adjust for indexing that starts at 0
    bids[0] = currentBid.amount;
    for(int i = 1; i < numOtherBids + 1; i++){
        bidTypes[i] = random() % numBidderTypes;
        bids[i] = invCDFs[bidTypes[i]][aucType][random() % 1000];
        //cout << aucType << "; " << bidTypes[i] << "; " << bids[i] << "\n";
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
    double topUtil = nlogitParams[6] + nlogitParams[3]*log(currentBid.numReps + 1) +
        nlogitParams[4]*buyRepVal + nlogitParams[5]*log(currentBid.previousCancels + 1);
    // Entry 6: c7_cons; entry 3: c7_lnnumreps; entry 4: c7_buyrep; entry 5: lnprevcancel

    // Traverse the array to add exp(utility) and find the max
    double expUtilSum = exp(utilities[0]);
    double utilitiesMax = utilities[0];
    for(int i = 1; i < numOtherBids + 1; i++){
        utilitiesMax = ( utilitiesMax < utilities[i] ? utilities[i] : utilitiesMax );
        expUtilSum += exp( utilities[i] );
    }
    double incVal = log( expUtilSum / exp(utilitiesMax) ) + utilitiesMax;

    // cout << "topUtil: " << topUtil << "; incVal: " << incVal << "; expUtilSum: " << expUtilSum << "\n";
    // cout << "utilities[0]: " << utilities[0] << "; utilitiesMax: " << utilitiesMax << "\n";
    
    double A = exp(topUtil + nlogitParams[8]*incVal);
    double B = exp(utilities[0] - utilitiesMax);
    double C = expUtilSum / exp( utilitiesMax );

    //printf("A: %lf. B: %lf. C: %lf.\n", A, B, C);

    // First entry is selection probability; second is its derivative
    pair<double, double> auctionResult;
    auctionResult.first = A/(1+A) * B/C;
    auctionResult.second = (  auctionResult.first * nlogitParams[1] * (B/(C*(1+A)) + 1/nlogitParams[8]*(1 - B/C)) );
    
    //printf("prob: %lf. probDer: %lf.\n", auctionResult.first, auctionResult.second);
    return( auctionResult );

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Main Program
// Strategy: import all data, then simulate 1000 auctions for each bid and each auction type. Use the mean
// results of the auction to find true selection probabilities and derivatives, then use those to infer
// seller costs.
int main(){

    //////////////////////////////////////////////////////////////////////////////
    //// Part 1: Import inverse CDFs and nested logit parameters
    
    // Use the getAucTraits function to get the number of bidder and auction types
    AucTraits aucTraits;
    aucTraits = getAucTraits();
    if( (aucTraits.numBidderTypes == 0) | (aucTraits.numAucTypes == 0) ){
        cout << "Error: zero bidder or auction types.\n";
        return(1);
    }

    // Import inverse CDF functions into a bidder_types x auction_types x 1000 vector (not an array since dimensions are unknown
    // at compile time)
    vector< vector< vector<double> > > invCDFs( aucTraits.numBidderTypes,
                                                vector< vector<double> >(aucTraits.numAucTypes, vector<double>(1000, 0)) );
    // Use a function (returning void) to import data to the invCDFs vector. Pass in the whole vector, but the function only
    // works with a reference to the memory address
    importInverseCDFs(invCDFs, aucTraits);

    // Import parameters as a vector (this restricts hard-coded changes to the function where they're used)
    vector<double> nlogitParams = importNLogitParams();


    ///////////////////////////////////////////////////////////////////////////////
    //// Part 2: simulate auctions for each bid in the data

    // Skip the header row and take the first bid
    FILE* bidFile = fopen("synthdata-weights2.txt", "r");
    char line[10000];
    char *res = fgets(line, sizeof(line), bidFile); // Gets header line, which we ignore

    // Get bids from other lines and process auctions    
    Bid currentBid;
    currentBid.isLastBid = false;

    // Number of times to simulate the auction
    int numAucSims = 1000;
    // Variables to calculate and store simulation outcomes
    double probSum;
    double probDerSum;
    pair<double, double> simulationResult;
    vector<double> emptyVec;
    vector< vector<double> > simulatedProb( aucTraits.numAucTypes, emptyVec );
    vector< vector<double> > simulatedProbDeriv( aucTraits.numAucTypes, emptyVec );
    vector< vector<double> > costs( aucTraits.numAucTypes, emptyVec );

    // Debugging: only run for the first 11 bids
    int bidCount = 0;
    // // DEBUGGING: Only run a single simulated auction with a known outcome
    // currentBid = getBidData(bidFile);
    // currentBid = getBidData(bidFile);
    // simulationResult = simulateAuction(currentBid, 0, aucTraits.numBidderTypes, invCDFs, nlogitParams);
    // simulationResult = simulateAuction(currentBid, 1, aucTraits.numBidderTypes, invCDFs, nlogitParams);
    // simulationResult = simulateAuction(currentBid, 2, aucTraits.numBidderTypes, invCDFs, nlogitParams);
    
    while( ! currentBid.isLastBid ){

        currentBid = getBidData(bidFile);
        
        // Simulate the bid 1,000 times for each auction type
        for(int aucType = 0; aucType < aucTraits.numAucTypes; aucType++){

            // Skip if this is a header bid with amount 0, but insert placeholders for all vectors
            if( currentBid.amount == 0 ){
                simulatedProb[aucType].push_back( -99 );
                simulatedProbDeriv[aucType].push_back( -99 );
                costs[aucType].push_back( -99 );
                continue;
            }
            
            // Reset simulation summary variables
            probSum = 0;
            probDerSum = 0;

            // Simulate results from the current bid numAucSims times, getting the selection probability and its derivative
            for(int i = 0; i < numAucSims; i++){
                simulationResult = simulateAuction(currentBid, aucType, aucTraits.numBidderTypes, invCDFs, nlogitParams);
                probSum += simulationResult.first;
                probDerSum += simulationResult.second;
            }
            //cout << (probSum / numAucSims) << "; " << (probDerSum / numAucSims) << "; " << (probSum / probDerSum) << "\n";
            // Store the averages and calculate the implied cost
            simulatedProb[aucType].push_back( probSum / numAucSims );
            simulatedProbDeriv[aucType].push_back( probDerSum / numAucSims );
            costs[aucType].push_back( .85*( currentBid.amount + (probSum / probDerSum) ) );
        }
        bidCount++;
        
    }

    // cout << simulatedProb[0][1] << "; " << simulatedProb[1][1] << "; " << simulatedProb[2][1] << "\n";
    // cout << costs[0][1] << "\n";

    // Write probabilities, derivatives, and costs to a CSV file with 3*aucTraits.numAucTypes columns
    // Note: this writes columns in reverse order compared to the original code
    ofstream outputFile;
    outputFile.open("costs.csv");

    for(int i = 0; i < simulatedProb[0].size(); i++){
        for(int aucType = 0; aucType < aucTraits.numAucTypes; aucType++){
            if(aucType > 0){
                outputFile << ", ";
            }
            outputFile << simulatedProb[aucType][i] << ", " << simulatedProbDeriv[aucType][i] << ", " <<
                costs[aucType][i];
        }
        outputFile << "\n";
    }
    outputFile.close();

    
    // Program finished execution: return normal exit code 0
    return 0;
}
