// Stata starts in project directory
// cd "~/work/DataAnalysis/NewProj/distclean"
cd "c:/Users/drewv/Documents/duke_econ/ra_work/bollinger_17_18/solar_beauty_contest_auctions/yoganarasimhan_simulation_code/"

insheet using synthdata-weights2.txt

gen decisiontype = 1 if decisionno == 1
replace decisiontype = 2 if decisionno != 1
label variable decisiontype "1 if cancel, 2 if bid"
label variable overalldecision "1 if cancel was chose, 2 if a bid was chosen"
label variable ttype "True type of the auction"
label variable aucwt "posterior prob. auction is of type 1"
label variable aucwt2 "posterior prob. auction is of type 2"
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


nlogitgen type = decisiontype(1, 2)

constraint 1 [type1_tau]_cons = 1

nlogit decision bidamount nsellrep || type: lnnumreps buyrep lnprevcancel, ///
base(1) estconst || decisionno: , noconstant case(bidrequestid) notree constraint(1)

mat coeff = e(b) // coefficient vector
mat2txt, m(coeff) sav(coeff.txt) replace // saves matrix coeff to a file called coeff.txt in the working directory
	
