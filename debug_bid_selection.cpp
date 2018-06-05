// debug_bid_selection.cpp
// A diagnostic for the functions modified in bid_selection.cpp so that they can be checked
// during editing.  This file uses many of the building blocks of calculate_costs.cpp, such
// as routines to import data.
// Drew Vollmer 2018-01-17

// Libraries imported in bid_selection.hpp
#include "bid_selection.hpp"


// Use standard namespace: can call cout and vector rather than std::cout and std::vector
using namespace std;


////////////////////////////////////////////////////////////
//// Class definitions

// Auction traits class storing the number of types used in the auction
// (Inferred from data files in getAucTraits() at runtime.)
typedef struct {
    int numBidderTypes;
    int numObsAucTypes;
    int numUnobsAucTypes;
} AucTraits;



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
        sprintf(fileName, "inv_cdf_bid%d_obsat1.csv", numBidderTypes + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numBidderTypes++;

            // Use the first file to get the number of columns in the data (unobserved auction types)
            if( numBidderTypes == 1 ){
                string firstLine;
                getline(infile, firstLine);
                aucTraits.numUnobsAucTypes = count(firstLine.begin(), firstLine.end(), ',') + 1;
            }
                
        } else {
            break;
        }
    }

    // After finding the number of bidder types, do the same thing to find the number of observed
    // auction types
    int numObsAucTypes = 0;
    while(true){
        sprintf(fileName, "inv_cdf_bid1_obsat%d.csv", numObsAucTypes + 1);
        ifstream infile(fileName);
        if( infile.good() ){
            // Increase number of files found
            numObsAucTypes++;                
        } else {
            break;
        }
    }
    
    
    aucTraits.numBidderTypes = numBidderTypes;
    aucTraits.numObsAucTypes = numObsAucTypes;
    // cout << "Bidder types: " << numBidderTypes << "; Observed auction types: " << numObsAucTypes << "; Unobserved auction types: " << aucTraits.numUnobsAucTypes << "\n";
    return( aucTraits );
}


// Import inverse CDFs
// Note that this function returns void, not the invCDFs 3D vector because of its size.  Instead,
// the function returns void and fills in the vector using a reference to its memory address.
void importInverseCDFs(vector< vector< vector< vector<double> > > >& invCDFs, AucTraits aucTraits){

    // Loop over all files; start by initializing variables used in the loops
    int lineNum;
    char fileName[100];
    string stringToRead;
    string line;
    
    for(int i = 0; i < aucTraits.numBidderTypes; i++){
        for(int j = 0; j < aucTraits.numObsAucTypes; j++){

            // Get file name using current name index
            sprintf(fileName, "inv_cdf_bid%d_obsat%d.csv", i + 1, j + 1);

            // Use ifstream to handle the unknown (at compile time) number of columns
            ifstream infile(fileName);
            // Ignore the first (header) row
            getline(infile, line);
        
            // Use a double-loop over known dimensions to fill in invCDFs[i][j]
            for(int row = 0; row < 1000; row++){
                for( int currentCol = 0; currentCol < aucTraits.numUnobsAucTypes; currentCol++){

                    // Read the [i][currentCol][row] entry from the file
                    infile >> stringToRead;

                    // Remove trailing commas, if there are any
                    stringToRead.erase( remove(stringToRead.begin(), stringToRead.end(), ','), stringToRead.end() );
                    // Convert to double and insert into invCDFs vector
                    invCDFs[i][j][currentCol][row] = atof( stringToRead.c_str() );
                }
            }
            // Finished processing the current file; next loop iteration handles the next file
        }
    }

}


int main(){

    // Set up using import routines
    // Use the getAucTraits function to get the number of bidder and auction types
    AucTraits aucTraits;
    aucTraits = getAucTraits();
    if( (aucTraits.numBidderTypes == 0) | (aucTraits.numUnobsAucTypes == 0) ){
        cout << "Error: zero bidder or auction types.\n";
        return(1);
    }

    // Import inverse CDF functions into a bidder_types x ObsAucTypes x UnobsAucTypes x 1000 vector
    // (not an array since dimensions are unknown at compile time)
    // vector< vector< vector<double> > > invCDFs( aucTraits.numBidderTypes,
    //                                             vector< vector<double> >(aucTraits.numUnobsAucTypes, vector<double>(1000, 0)) );
    vector< vector< vector< vector<double> > > > invCDFs( aucTraits.numBidderTypes,
                    vector< vector< vector<double> > >(aucTraits.numObsAucTypes,
                                                    vector< vector<double> >(aucTraits.numUnobsAucTypes, vector<double>(1000, 0)) ) );
    // Use a function (returning void) to import data to the invCDFs vector. Pass in the whole vector, but the function only
    // works with a reference to the memory address
    importInverseCDFs(invCDFs, aucTraits);


    ///////////////////////////////////////////////////////////////////////////////////////////////
    //// Debugging: parameter import
    
    // Import parameters as a vector (this restricts hard-coded changes to the function where they're used)
    BidSelectionParams nlp = getBidSelectionParams();


    ///////////////////////////////////////////////////////////////////////////////////////////////
    //// Debugging: bid import and auction simulation
    
    // Skip the header row and take the first bid
    FILE* bidFile = fopen("template_data.csv", "r");
    char line[10000];
    char *res = fgets(line, sizeof(line), bidFile); // Gets header line, which we ignore

    // Get bids from other lines and process auctions    
    Bid currentBid;
    currentBid.isLastBid = false;

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

        for(int j = 0; j < aucTraits.numUnobsAucTypes; j++){
            simulationResult = simulateAuction(currentBid, j, aucTraits.numBidderTypes, invCDFs, nlp);
        }

        if( bidCount > 10 ){
            break;
        }
        
        bidCount++;

    }
    
    // Program finished execution: return normal exit code 0
    return 0;
}

