// estimate_bid_selection.do
// Estimating a nested logit model used to judge the probability that each bid wins an auction
// Written by Hema Yoganarasimhan
// Lightly adapted by Drew Vollmer 2018-01-09


// Stata starts in project directory

insheet using "template_data.csv"

gen decisiontype = 1 if bidamount == 0
replace decisiontype = 2 if bidamount != 0
label variable decisiontype "1 if cancel, 2 if bid"
label variable overalldecision "1 if cancel was chosen, 2 if a bid was chosen"

gen nsellrep = sellrep - 8
replace nsellrep = 0 if decisiontype == 1
label variable nsellrep "sellrep - 8"
gen lnnumreps = ln(numreps+1)
label variable lnnumreps "ln(numreps+1)"
gen buyrep = 0
replace buyrep =  sumrep/numreps if numreps != 0
label variable buyrep "sumrep/numreps if numreps != 0, else 0"
gen lnprevcancel = ln(previouscancels+1)
label variable lnprevcancel "ln(previouscancels+1)"

/* Create ID within auctions */
by auctionid: gen decisionno = _n

/* Define nest for nested logit: observations with decisiontype 1 and */
/* decisiontype 2 are contained in different nests */
nlogitgen type = decisiontype(1, 2)

constraint 1 [type1_tau]_cons = 1

/* Syntax: bidamount and nsellrep determines choice between bids. lnnumreps, buyrep, */
/* and lnprevcancel are constant for each bid and determine the choice between type 1 */
/* (accepting a bid) and the outside option */
nlogit decision bidamount nsellrep || type: lnnumreps buyrep lnprevcancel, ///
base(1) estconst || decisionno: , noconstant case(auctionid) notree constraint(1)

mat coeff = e(b) // coefficient vector
mat2txt, m(coeff) sav(coeff.txt) replace // saves matrix coeff to a file called coeff.txt in the working directory
