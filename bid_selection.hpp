// bid_selection.h
// Declarations for the bid selection functions that the user edits when adapting and running
// the beauty contest auction estimation
// Drew Vollmer 2018-01-17

// Header guards: make sure that the header isn't loaded twice
#ifndef BID_SELECTION_INCLUDED
#define BID_SELECTION_INCLUDED

// For convenience, load all necessary libraries here
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
#include <numeric> // For accumulate()

// Declarations
// Note that the standard namespace isn't (and shouldn't) be used in the header file, so some data
// types need a std:: prefix that they don't have in .cpp files with using namespace std;

// Bid data type
typedef struct {
    double amount;
    int bidderType;
    int sumRep;
    int numReps;
    int previousAuctions;
    int previousCancels;
    int obsAucType;
    bool isLastBid; // Tells if this bid is the last one in the file
} Bid;

// Bid selection model coefficient data type
typedef struct {
    double bidAmountCoeff;
    double sellRepCoeff;
    double nestCorr;
    double nestConstant;
    double lnnumrepsCoeff;
    double buyrepCoeff;
    double lnprevcancelCoeff;
} BidSelectionParams;

// Auction traits class storing the number of types used in the auction
// (Inferred from data files in getAucTraits() at runtime.)
typedef struct {
    int numBidderTypes;
    int numObsAucTypes;
    int numUnobsAucTypes;
} AucTraits;


// Function to import bid data from each row of a file
Bid getBidData(std::FILE *bidFile);

// Function to import bid selection model parameters
BidSelectionParams getBidSelectionParams();

// Function to import distribution of bidder types
std::vector< std::vector<double> > importBidderTypeDist(AucTraits aucTraits);
// Function to import distribution of number of bidders
std::vector< std::vector<double> > importNumBidDist(AucTraits aucTraits);

// Function to simulate an auction
std::pair<double, double> simulateAuction(Bid currentBid, int uAucType, int numBidderTypes,
                                          std::vector< std::vector< std::vector< std::vector<Bid> > > >& sampleBids,
                                          BidSelectionParams& bidSelParams,
                                          std::vector< std::vector<double> > bidderTypeCumDist,
                                          std::vector< std::vector<double> > numBidCumDist);


// End header guard with endif statement
#endif
