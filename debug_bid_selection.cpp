// debug_bid_selection.cpp
// A diagnostic for the functions modified in bid_selection.cpp so that they can be checked
// during editing.  This file uses many of the building blocks of calculate_costs.cpp, such
// as routines to import data.
// Compilation command:
// g++ -o debug_bid_selection.exe debug_bid_selection.cpp bid_selection.cpp
// Drew Vollmer 2018-01-17

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

// Import the distribution of bidder types in each observed type of auction
vector< vector<double> > importBidderTypeDist(AucTraits aucTraits){

    // Open the file and look for aucTraits.numObsAucTypes rows
    ifstream infile("bidder_type_distribution.csv");

    // Declare the output array
    vector<double> emptyVec;
    vector< vector<double> > bidderTypeDist(aucTraits.numObsAucTypes, emptyVec);
    vector< vector<double> > bidderTypeCumDist(aucTraits.numObsAucTypes, emptyVec);


    // Read each cell into the string readProb
    string readProb;
    // Traverse rows and columns
    for(int row = 0; row < aucTraits.numObsAucTypes; row++){
        for(int col = 0; col < aucTraits.numBidderTypes; col++){

            // Take up to the next delimiter (comma unless it's the last column)
            if( col < aucTraits.numBidderTypes - 1 ){
                getline(infile, readProb, ',');
            } else {
                getline(infile, readProb, '\n');
            }

            // Define the probability of each bin and the cumulative probability
            bidderTypeDist[row].push_back( atof(readProb.c_str()) );
            bidderTypeCumDist[row].push_back( accumulate(bidderTypeDist[row].begin(), bidderTypeDist[row].end(), 0.0) );
        }
    }

    // Return the cumulative probability since it's easier to compute with
    return( bidderTypeCumDist );
}


// Import the distribution of the number of bids in each observed type of auction
vector< vector<double> > importNumBidDist(AucTraits aucTraits){

    // Open the file and look for aucTraits.numObsAucTypes rows
    ifstream infile("num_bid_distribution.csv");

    // Get the number of columns in the file
    string firstLine;
    getline(infile, firstLine);
    int numCols = count(firstLine.begin(), firstLine.end(), ',') + 1;
    // Reset to the start of the file
    infile.seekg(0, ios::beg);
    
    // Declare the output array
    vector<double> emptyVec;
    vector< vector<double> > numBidDist(aucTraits.numObsAucTypes, emptyVec);
    vector< vector<double> > numBidCumDist(aucTraits.numObsAucTypes, emptyVec);


    // Read each cell into the string readProb
    string readProb;
    // Traverse rows and columns
    for(int row = 0; row < aucTraits.numObsAucTypes; row++){
        for(int col = 0; col < numCols; col++){

            // Take up to the next delimiter (comma unless it's the last column)
            if( col < numCols - 1 ){
                getline(infile, readProb, ',');
            } else {
                getline(infile, readProb, '\n');
            }

            // Define the probability of each bin and the cumulative probability
            numBidDist[row].push_back( atof(readProb.c_str()) );
            numBidCumDist[row].push_back( accumulate(numBidDist[row].begin(), numBidDist[row].end(), 0.0) );
        }
    }
    
    // Return the cumulative probability since it's easier to compute with
    return( numBidCumDist );
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

    cout << "Number of bidder types: " << aucTraits.numBidderTypes << "\n";
    cout << "Number of unobservable auction types: " << aucTraits.numUnobsAucTypes << "\n";
    cout << "Number of observable auction types: " << aucTraits.numObsAucTypes << "\n";
    
    // Import sample bids as a bidder_types x ObsAucTypes x UnobsAUcTypes x 10000 vector
    // (Dimensions unknown at compile time)
    Bid emptyBid;
    vector< vector< vector< vector<Bid> > > > sampleBids( aucTraits.numBidderTypes,
            vector< vector< vector<Bid> > >( aucTraits.numObsAucTypes,
                    vector< vector<Bid> >(aucTraits.numUnobsAucTypes, vector<Bid>(10000, emptyBid)) ) );    
    // Use a function (returning void) to import the bids. (Function takes the whole vector as an
    // argument, but only works with a reference to the memory address.)
    importSampleBids(sampleBids, aucTraits);

    
    // Import parameters
    BidSelectionParams nlp = getBidSelectionParams();

    // Import distribution of bidder types
    vector< vector<double> > bidderTypeCumDist = importBidderTypeDist(aucTraits);
    // Import distribution of number of bids
    vector< vector<double> > numBidCumDist = importNumBidDist(aucTraits);


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

        // Outside option bids have bidder type zero.  Don't simulate auctions for these.
        if( currentBid.bidderType == 0 ){
            continue;
        }

        for(int j = 0; j < aucTraits.numUnobsAucTypes; j++){
            simulationResult = simulateAuction(currentBid, j, aucTraits.numBidderTypes, sampleBids,
                                               nlp, bidderTypeCumDist, numBidCumDist);
        }

        if( bidCount > 10 ){
            break;
        }
        
        bidCount++;

    }
    
    // Program finished execution: return normal exit code 0
    return 0;
}
