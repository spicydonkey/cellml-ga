# Development log for CellML GA-based code

## 08.06.2015
### INFINITE valued genomes
* simulations returning INF are appropriately tagged invalid
* However output shows non-convergence of population
* BUG in select_weighted function in GAEngine.h
* test function: inf_schwefel
* Bug seems to be fixed by assigning summand of an invalid genome to 0 not 99999... 

### Negative valued genomes
* GA implementaion in GAEngine.h expects non-negative fitness
* test function: neg_schwefel
* Negative fit genomes are correctly ranked higher in population but never selected?
* 0 fit genomes are assigned 1.0/0.00...01 in calculation of cum-sum which is reasonable
* edit print_stage to tag negative fitness genomes with "!"

### INFINITE vs very large (~999999999.9999) valued fitness
* inf_schwefel tests never show the population (nor fittest)
* converge with original GA implementation
* lge_schwefel tests converge to the global minimum; large
* genomes indeed seen at the bottom of the ranked pop list
	
### INFINITE valued genome bug has been fixed

### Redundant attributes
* InitialPopulation and MaxPopulation are redundant attributes in this aplication of GA
* MaxPopulation is not used at all by the program to "set border" to population
* InitialPopulation value is used as population for GA; may require alt naming. e.g. Population
	
### TODO
* [ ] experiment.cpp
  * [ ] unused VariableHolder "params"
  * [ ] var_template's dynamics in program
  * [ ] unreferenced object "comps"

* [ ] GAEngine.h
  * [ ] Genome::same function uses bitwise & on bools
  * [ ] Each generation's fittest should be output as a summary
  * [ ] Multiple-degree CROSSOVER can be dangerous!
  * [ ] build_rnd_sample is a core routine in GA - check for validity

## 09.06.2015
### Git version control
* Unable to get connection to remote repo on GitHub from Cluster

### Fixes to GAEngine.h
* Genome::same contained bitwise AND where logical AND more appropriate
* GAEngine::print_stage to output fittest genome in each generation

verbosity	|	output
----------------|----------------
0		| fittest run
1		| fittest gen & fittest so far & fittest run
1<		| pop of each gen & fittest run

* modify print_stage output:

verbosity	|	output
----------------|----------------
0		| fittest in the whole run (NO CHANGES)
1		| **fittest of each generation & fittest in the whole run**
1<		| population of each generation & fittest in the whole run (NO CHANGES)

### Add detailed feedback on genetic operations for verbosity > 2
* Selection: printf("Adding %d to population\n",mem);

### TODO
* [ ] Refine genetic operators code
* [ ] Feedback of GO behaviour with 2< verbosity

## 10.06.2015
### Clean-up select_weighted code
```
GA is implemented to minimisation of *non-negative* fitness, so selection weight of 1/fitness is assigned.
Exceptions occur for invalid genomes, which are assigned (bug) NULL (999~999.999) weight, and global-minima which replace fitness in previous calculation with 0.0~01.
Such *magic numbers* are to be cleaned-up.
```
### 
