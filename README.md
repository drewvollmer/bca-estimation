README.md
---------

The layout for beauty contest auction estimation

Yoganarasimhan's estimation creates a random sample, then runs:
sortkde3.cc
synthdata.do
simulate3-clean.cc

Break down each file into what has to be done:

### sortkde3.cc

- Reads in data from sampled-aucs.txt
- Iterates over Kernel density until UpdateS() is within tolerance
- Writes data to synthdata-weights2.txt


### synthdata.do