// calculate_costs.cpp
// Simulate auctions for each bid; use the resulting choice probabilities to infer seller costs
// Compiled as: g++ -o calculate_costs.exe calculate_costs.cpp bid_selection.cpp
// Drew Vollmer 2017-12-22

// Libraries imported in bid_selection.hpp
#include "bid_selection.hpp"

// Use standard namespace: can call cout and vector rather than std::cout and std::vector
using namespace std;

////////////////////////////////////////////////////////////
//// Class definitions

// Contained in bid_selection.hpp



///////////////////////////////////////////////////////////////////////////////////
//// Functions

//// Functions to import data

// Get number of bidder types and auction types in the data
AucTraits getAucTraits(){

    // Declare variable to return
    AucTraits aucTraits;

    // Assuming CDFs are in files of the form "inv_cdf_bid[0-9]_obsat[0-9].csv", find the number of bidders
    // and thus the number of files to import. (CDF files are for pairs of bidder types and observed auction types.)
    int numBidderTypes = 0;
    char fileName[50];
    size_t numCols;

    // Strategy: count the files that exist and break after the first file that does not exist
    while(true){
        sprintf(fileName, "sample_bids_btype_%d_oauctype_1_uauctype_1.csv", numBidderTypes + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numBidderTypes++;
        } else {
            break;
        }
    }

    // After finding the number of bidder types, do the same thing to find the number of observed
    // auction types and unobserved auction types
    int numObsAucTypes = 0;
    while(true){
        sprintf(fileName, "sample_bids_btype_1_oauctype_%d_uauctype_1.csv", numObsAucTypes + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numObsAucTypes++;                
        } else {
            break;
        }
    }

    int numUnobsAucTypes = 0;
    while(true){
        sprintf(fileName, "sample_bids_btype_1_oauctype_1_uauctype_%d.csv", numUnobsAucTypes + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numUnobsAucTypes++;
        } else {
            break;
        }
    }
        
    aucTraits.numBidderTypes = numBidderTypes;
    aucTraits.numObsAucTypes = numObsAucTypes;
    aucTraits.numUnobsAucTypes = numUnobsAucTypes;
    // cout << "Bidder types: " << numBidderTypes << "; Observed auction types: " << numObsAucTypes << "; Unobserved auction types: " << aucTraits.numUnobsAucTypes << "\n";
    return( aucTraits );
}


// Import sample bids
// Helper function to import the next bid
Bid getSampleBid(FILE *sampleBidFile){

    char line[10000];
    char *res = fgets(line, sizeof(line), sampleBidFile);

    // Declare bid object to return and fill with the current line
    Bid currentBid;

    sscanf(line, "%lf, %d, %d, %d, %d, %lf, %lf", &currentBid.amount, &currentBid.bidderType,
           &currentBid.loanOffered, &currentBid.usPanel, &currentBid.chinesePanel,
           &currentBid.loanRate, &currentBid.relativeSystemSize);
    
    return( currentBid );
}
// Note that this function returns void, not the sampleBids vector, because of its size.  Instead,
// the function returns void and fills in the vector using a reference to its memory address.
void importSampleBids(vector< vector< vector< vector<Bid> > > >& sampleBids, AucTraits aucTraits){

    // Loop over all files; start by initializing variables used in the loops
    int lineNum;
    char fileName[100];
    string stringToRead;
    string line;
    
    for(int i = 0; i < aucTraits.numBidderTypes; i++){
        for(int j = 0; j < aucTraits.numObsAucTypes; j++){
            for(int k = 0; k < aucTraits.numUnobsAucTypes; k++){

                // Get file name using current name indices
                sprintf(fileName, "sample_bids_btype_%d_oauctype_%d_uauctype_%d.csv",
                        i + 1, j + 1, k + 1);
                cout << "Importing file " << fileName << "\n";

                // Use ifstream
                FILE* sampleBidFile = fopen(fileName, "r");
                char line[10000];
                char *res = fgets(line, sizeof(line), sampleBidFile); // Header line, which we ignore

                // Import in a loop
                for(int row = 0; row < 10000; row++){
                    sampleBids[i][j][k][row] = getSampleBid(sampleBidFile);
                }
                                
                // Finished processing the current file; next loop iteration handles the next file
            }
        }
    }

}

// Import nested logit parameters:
// Defined in bid_selection.cpp





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
    if( (aucTraits.numBidderTypes == 0) | (aucTraits.numUnobsAucTypes == 0) ){
        cout << "Error: zero bidder or auction types.\n";
        return(1);
    }

    // Import sample bids as a bidder_types x ObsAucTypes x UnobsAUcTypes x 10000 vector
    // (Dimensions unknown at compile time)
    Bid emptyBid;
    vector< vector< vector< vector<Bid> > > > sampleBids( aucTraits.numBidderTypes,
            vector< vector< vector<Bid> > >( aucTraits.numObsAucTypes,
                    vector< vector<Bid> >(aucTraits.numUnobsAucTypes, vector<Bid>(10000, emptyBid)) ) );    
    // Use a function (returning void) to import the bids. (Function takes the whole vector as an
    // argument, but only works with a reference to the memory address.)
    importSampleBids(sampleBids, aucTraits);

    // Import parameters as a vector (this restricts hard-coded changes to the function where they're used)
    BidSelectionParams nlp = getBidSelectionParams();


    ///////////////////////////////////////////////////////////////////////////////
    //// Part 2: simulate auctions for each bid in the data

    // Skip the header row and take the first bid
    FILE* bidFile = fopen("template_data.csv", "r");
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
    vector< vector<double> > simulatedProb( aucTraits.numUnobsAucTypes, emptyVec );
    vector< vector<double> > simulatedProbDeriv( aucTraits.numUnobsAucTypes, emptyVec );
    vector< vector<double> > costs( aucTraits.numUnobsAucTypes, emptyVec );

    // Debugging: only run for the first 11 bids
    int bidCount = 0;
    
    while( ! currentBid.isLastBid ){

        currentBid = getBidData(bidFile);
        
        // Simulate the bid 1,000 times for each auction type
        for(int uAucType = 0; uAucType < aucTraits.numUnobsAucTypes; uAucType++){

            // Skip if this is an outside option bid, but insert a placeholder
            if( currentBid.bidderType == 0 ){
                simulatedProb[uAucType].push_back( -99 );
                simulatedProbDeriv[uAucType].push_back( -99 );
                costs[uAucType].push_back( -99 );
                continue;
            }
            
            // Reset simulation summary variables
            probSum = 0;
            probDerSum = 0;

            // Simulate results from the current bid numAucSims times, getting the selection probability and its derivative
            for(int i = 0; i < numAucSims; i++){
                simulationResult = simulateAuction(currentBid, uAucType, aucTraits.numBidderTypes, sampleBids, nlp);
                probSum += simulationResult.first;
                probDerSum += simulationResult.second;
            }
            // cout << (probSum / numAucSims) << "; " << (probDerSum / numAucSims) << "; " << (probSum / probDerSum) << "\n";
            // Store the averages and calculate the implied cost
            simulatedProb[uAucType].push_back( probSum / numAucSims );
            simulatedProbDeriv[uAucType].push_back( probDerSum / numAucSims );

            // Costs need to be multiplied by 1 - commission to be accurate
            costs[uAucType].push_back( currentBid.amount + (probSum / probDerSum) );
        }
        bidCount++;        
    }

    // Write probabilities, derivatives, and costs to a CSV file with 3*aucTraits.numUnobsAucTypes columns
    ofstream outputFile;
    outputFile.open("estimated_costs.csv");

    for(int i = 0; i < simulatedProb[0].size(); i++){
        for(int uAucType = 0; uAucType < aucTraits.numUnobsAucTypes; uAucType++){
            if(uAucType > 0){
                outputFile << ", ";
            }
            outputFile << simulatedProb[uAucType][i] << ", " << simulatedProbDeriv[uAucType][i] << ", " <<
                costs[uAucType][i];
        }
        outputFile << "\n";
    }
    outputFile.close();

    
    // Program finished execution: return normal exit code 0
    return 0;
}
