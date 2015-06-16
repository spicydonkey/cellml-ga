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
* [x] experiment.cpp
  * [x] unused VariableHolder "params"
  * [x] var_template's dynamics in program
  * [x] unreferenced object "comps"

* [x] GAEngine.h
  * [x] Genome::same function uses bitwise & on bools
  * [x] Each generation's fittest should be output as a summary
  * [x] Multiple-degree CROSSOVER can be dangerous!
  * [x] build_rnd_sample is a core routine in GA - check for validity

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
* [x] Refine genetic operators code
* [x] Feedback of GO behaviour with 2< verbosity

## 10.06.2015
### Clean-up select_weighted code
```
GA is implemented to minimisation of *non-negative* fitness, so selection weight of 1/fitness is assigned.
Exceptions occur for invalid genomes, which are assigned (bug) NULL (999~999.999) weight, and global-minima which replace fitness in previous calculation with 0.0~01.
Such *magic numbers* are to be cleaned-up.
```
### Crossover
* Reference to *build_rnd_sample* to set up arena for breeding
  * [x] fix invalid-passing bug in build_rnd_sample
* The multiple Xover issue is definitely present, but unlikely to be harmful to GA and left with a TODO tag

```
Caution:
The population selected before crossover does not contain any invalid genomes.
Crossover **can** produce invalid genomes immediately synced by the population.
Not all steps were taken to eliminate run-time errors arising from the whole population turning invalid.
Limitations surely exist for simulations that can frequently breakdown.
```

### Mutation
* Unknown reason for line *m_Population[sample[i]].set(v);*

## 11.06.2015
### Update output for more verbose setting
The dynamics of individual genome in population should be *visible* to the user for a more verbose setting. 
* [x] Selection

```
//output format

SELECT: Adding 'genome_index' to population
```

```
#ifdef SEQMODE
					// Genetic operator feedback
					if(verbosity>2)
						printf("SELECT: Adding %d to population\n",mem);
#else
```

* [x] Crossover

```
//output format

CROSSOVER:
-[i] x1=________   x2=______   ...   xN=________
-[j] x1=________   x2=______   ...   xN=________
+[i] x1=________   x2=______   ...   xN=________
+[j] x1=________   x2=______   ...   xN=________
--------------------------------------------------
```

* [x] Mutation

```
//output format

MUTATION:
-[i] x1=_________ x2=__________
+[i] x1=_________ x2=__________
-----------------------------------
```

### Create genome sequencing member function
* [x] *print_genome(int index_to_genome);* to output a genome's alleles and corresponding values in order

## 12.06.2015
### Complete GO feedback functionality
Added user-feedback functionality to observe the dynamics of the genetic algorithm, namely at the *Genetic Operations* (GO).
User needs to ask for a very-verbose run of the experiment (greater than 2) to activate GO feedback. 
Dynamics of the individual genome/population at the Selection, Crossover, and Mutation stages are output appropriately.

**NOTE**: Genetic operators act on genomes in 'current' population, accessing population members via index.
At this stage, care should be taken in interpreting genome's index from the output, since the population summary is effectively for the 'previous' generation.
The selection operation is exactly the stage where a gene-pool for a new generation is picked. Hence, the index labelling the genomic sequence in this functionality do not correspond to the preceding population summary.

verbosity	|	output
----------------|----------------
0		| fittest in the whole run 
1		| fittest of each generation & fittest in the whole run
2		| population of each generation & fittest in the whole run
2<		| population & **Genetic Operators** & fittest in run

### Genetic Algorithm Tests
#### Parameters
* Population size
* Mutation rate
* Crossover rate
* Generation
* ~Sampling~ *default to non-block sampling*
* Genome length: solution space dimension
* Allele limits
* Allele names

```
NOTE: parameters listed above show significant interaction in a GA
```

1. No mutation and no crossover
  * nomutacross.xml

## 15.06.2015
### Meeting minutes
* To finish testing ported GA (sequentialised version) for any overseen bugs
* Running CellML simulations on OpenCOR and exporting *csv* output
* Alleles from CellML to be non-negative
* Fitness evaluation likely to be non-negative when integrating with CellML (squared residual like method)
* Model target variable to compare to VE defined in XML's **ResultColumn** attribute
* ResultColumn variable can be set appropriately by looking at the *csv* file

### GA Tests continued
Test 0: No mutation and no crossover
  * 0-nomutanox.xml
    * [x] Initial population is randomly scattered
	* [x] Fittest genome does not always survive (selection to create gene pool give equal weights to all members)
	* [x] No *new* genome is seen after the initial population
    * [x] The best fitness per generation is non-decreasing
	* [x] Population converges
	* [x] As soon as the population converges the population is stationary

Test 1: No mutation and low crossover (1%)
  * 1-nomutalowx.xml
    * [x] Initial population is randomly scattered
	* [x] New genomes created - via crossover
	* [x] Crossover feedback
	  * Between 0 and 4 crossover operations are performed per-generation, with popsize 100, and mutation_proportion 0.01
	* [x] Observe best fitness per generation improve at times
	* [ ] Population converges and stays stationary (without mutation) **for sufficiently large generation #**
	  * Not enough generations to allow population to converge
	* [x] Can get solutions better than initially scattered population
	  * Not global optimum
  * 1-nomutalowx-1.xml
    * [x] Population converges and stays stationary
	  * When the population converges, the any crossover operations henceforth are effectively self-crossing, and do not create new genomes
	  * Stuck in local minimum

Test 2: No mutation and high crossover (25%)
  * 2-nomutahighx.xml
    * [x] Initial population randomly scattered
	* [x] New genomes *often* created - frequent crossover
	* [x] Crossover feedback
	* [x] Best fitness per generation fluctuate more often
	* [ ] Population converges then stationary
	  * more generations required
	* [x] Get solutions better than initial population
	* [ ] More generations to achieve convergence
	  * more generations required
  * 2-nomutahighx-1.xml
    * [x] Population converges then stationary
	* [ ] More generations to achieve convergence
	  * difference is not significant

**Henceforth, tests are simplified to 5-Dimensional Schwefel function**

Test 3: Low mutation (5%) and no crossover
  * 3-lowmutanox.xml
    * [x] Initial population randomly scattered
	* [x] New genomes are consistently created (5% mutation)
	* [x] Mutation feedback
	* [x] On average, 1 allele is mutated in a selected genome
	* [ ] Population converges, but genome dynamics due to mutation
	  * Population seems to converge in less than 100 generations for this test, but require larger gen# to make sure
	  * The effect of mutation is clearly visible even when the population contains a dominant converged region
	* [x] Get solutions better than initial population
	* Other features
	  * could not recognise any other distinct features
  * 3-lowmutanox-1.xml (Generations=1000)
    * [ ] Population converges, but genome dynamics due to mutation
	  * **Population has not converged!**
	  * Seems like the global minimum can be found for a sufficiently long run
  * **3-lowmutanox-2.xml** (Generations=10000)
    * [x] Convergence to global minimum
	  * convergence to ~0.01 fitness reached with 5-dim
	  * **This is quite surprising but not completely unexpected**

```
Explanation:
1. At a certain generation, the population is dominated by a certain genome
2. Low rate mutation creates a fitter genome (likely mutated from the dominant)
3. The mutated genome is likely to survive, having higher weight, and multiply in gene-pool
4. The population pseudo-converges with the mutated genome being dominant
5. Repeat from 1
```

## 16.06.2015
### GA Tests continued

Test 4: High mutation (25%) and no crossover
  * 4-highmutanox.xml
    * [x] Initial population randomly scattered
	* [x] New genomes are very often created
	* [x] Mutation feedback
	* [x] 1 allele mutate on average per mutating genome
	* [ ] Population heavily oscillatory (shouldn't see convergence)
	  * Selection process should not be able to weed out unfit genomes and select fit genomes fast enough for mutation
	  * In fact, there is clearly a *dominant* gene in population, and in a weak-sense, the population has converged to this gene; the population should converge to global minimum by a similar reasoning to test-3
	* [x] Get better solutions than initial population
	* Other features
	  * No other distinct features
  * 4-highmutanox-1.xml (Generations=10000)
    * [x] Convergence to global minimum
	  * gen 100: 9.6
	  * gen 500: 0.91
	  * gen 2500: 0.021
	  * gen 10000: 0.0010

Test 5: Mutation and crossover
  * 5-0-mutax.xml (mut/x: 0.1/0.7)
  * 5-1-mutax.xml (0.25/0.7)
  * 5-2-mutax.xml (0.25/0.25)