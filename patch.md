# Patch notes (v1.0)

### Verbosity [-v]
User specifies how verbose the output of program should be at the call of *experiment*.

Number of [-v] flags entered at the call to the experiment executable determines the verbosity of the output.

The summary of output for user-specified verbosity is given in the table below:

verbosity	|	output
----------------|----------------
0		| Fittest genome of the run 
1		| Fittest genome in each generation + Fittest genome of the run
2		| Population at each generation + Fittest genome of the run 
3		| + Population at each genetic operator (GO) stage (selection, crossover, mutation)
4		| + Individual GO 
