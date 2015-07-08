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
#### Test 0: No mutation and no crossover
  * 0-nomutanox.xml
    * [x] Initial population is randomly scattered
	* [x] Fittest genome does not always survive (selection to create gene pool give equal weights to all members)
	* [x] No *new* genome is seen after the initial population
    * [x] The best fitness per generation is non-decreasing
	* [x] Population converges
	* [x] As soon as the population converges the population is stationary

#### Test 1: No mutation and low crossover (1%)
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

#### Test 2: No mutation and high crossover (25%)
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

#### Test 3: Low mutation (5%) and no crossover
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

#### Test 4: High mutation (25%) and no crossover
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

#### Test 5: Mutation and crossover
  * 5-0-mutax.xml (mut/x: 0.1/0.7)
  * 5-1-mutax.xml (0.25/0.7)
  * 5-2-mutax.xml (0.25/0.25)
  * 5-3-mutax.xml (0.4/0.7)

On average, the population converges to fitness of ~10 from the above runs (pop=100 gen=100), which is not too bad.

#### Test 6: INFINITE fitness case

##### Description
  * Test function is **inf_schwefel** defined in *GATESTER.h* header that returns INFINITE when the allele values fall in a certain range

##### Expected behaviour
  * INFINITE-valued genomes are appropriately labelled *invalid* and tagged with \* in population feedback
  * INFINITE-valued genomes should be *weeded-out* as the generation passes, since the gene-pool selection ignores invalid genomes and gives less weighting for higher fitness

##### Test files
  * 6-0-inf.xml
    * output clearly shows GA follows the expected behaviour outlined above
	* a simple run (params=5 pop=100 gen=100) resulted in the latest gen convergence to ~5 fitness

## 17.06.2015
### Patch
Apply debugging changes made to the *sequential version* back on the original codebase, fixing the INFINITE-fit genome breeding issue and random selection algorithm, in particular.

#### Original codebase
The original codebase for the CellML-GA project loaded onto the **patch** branch in git repository, including files:

  * distributor.cpp
  * distributor.h
  * cellml_observer.h

#### Patch-log
(1) experiment.cpp|return to distributed master-slave code
```c++
/*	COMMENTING THIS BLOCK FOR PATCH SUFFICES FOR THIS FILE
// define SEQMODE to build a sequential mode to test code (Schwefel function); no call to CellML
#define SEQMODE
*/
```

(2) GAEngine.h|Genetic-operator feedback features
```c++
// Genetic operator feedback
if(verbosity>2)
	printf("SELECT: Adding %d to population\n",mem);
```
etc...

(3) experiment.cpp|PATCH global to compile patched code

(4) GAEngine.h|Warning tag displayed for negative fitness (GA assumes non-negative fitness evaluator)

(5) GAEngine.h|build_rnd_sample|Debug a loop-hole
```c++
...
				// randomly assign an int to v 
				// if reject_duplicates set true, add a unique index to sample
				// if check_valid set true, add a valid index
                do
                {
#ifndef PATCH
					v=(int)(rnd_generate(0.0,limit));	// TODO v in [0, m_Pop.size()-1] - slightly disfavours the last index

					// if check_valid is set true, loop until v is a valid Genome
					// TODO (BUG - invalid genomes will still be pushed back onto sample)
                    if(check_valid && !m_Population[v].valid())
						continue;			
#else
					// nested do-while until genome is both valid and unique
					do
					{
						v=(int)(rnd_generate(0.0,limit));
					} while (check_valid && !m_Population[v].valid());
#endif
                } while(reject_duplicates && std::find(sample.begin(),sample.end(),v)!=sample.end());
                //Found next genome
				...
```

(6) GAEngine.h|select_weighted|Fix selection bug
```c++
#ifdef PATCH
				sum+=(p[i].valid()?1.0/(p[i].fitness()?p[i].fitness():zero_lim):0.0);
#else
				sum+=(p[i].valid()?1.0/(p[i].fitness()?p[i].fitness():0.000000000001):99999999999.99999);
#endif
```

(7) distributor.h|included by the parallelised version, but no changes need to be made

(8) distributor.cpp|no changes; clean-up

(9) virtexp.h & virtexp.cpp|no changes; clean-up

(10) utils.h & utils.cpp|no changes

(11) makefile-patch|makefile for compiling the patched version (Intel's mpi compiler)

## 19.06.2015
### Compiling the patched project
* **mpiicpc** compiler (at the time of writing is used to compile the c++ code with MPI) requires the module: **intel/ics-2013**

### Test directories
The database hierarchy for tests are as follows:
```
PROJECT_DIRECTORY(=/projects/uoa00322/david.shin)
	|
	|____tests
		|
		|____seqmode
		|
		|____patch
			|
			|__0-ThisModel
			|__1-ThatModel
			|...
```

### CellML Test Models
#### ShiftedSchwefel_2D
Search a positive domain [1,1000]

#### Schwefel_2D
##### Description
Search domain [-500,500]

A dummy ODE is solved but the *objective* variable is time-independent 

Knowing the behaviour of the Schwefel function, target of 0.0 (function value at global minimum) is set at an arbitrary time (2.57s in this case)

##### Results
Population feedback shows all genomes' fitnesses are evaluated to **INFINITE**!

As expected, *crossover* never gets performed, but the whole gene-pool gets *mutated*.

An interesting behaviour is seen at the *selection* procedure initialising the gene-pool: The first (0th) genome in population **completely fills* the gene-pool!
```
...
SELECT: Adding 0 to population
SELECT: Adding 0 to population
SELECT: Adding 0 to population
...
```

#### Ramp_2D

#### ip3model

### TODO
* Problem: SEQMODE flag needs to be manually switched by the user before running make on corresponding project version, which makes switching versions a tedious task

## 22.06.2015
### Clean code
* [x] Clean SEQMODE codes in patch branch
* [x] release a SEQMODE version from the current master branch

#### Schwefel_2D
* Try non-zero target, like the shifted schwefel test, such as 1.0
  * No INFINITE alleles seen with non-zero target
  * Crossover and Mutation seen
  * Population converge to global minimum (100th Gen)
    * Fitness=0.005 Alleles=420.96,420.22  
* Try different time and target: 10.0s and 0.1
  * No INFINITE alleles seen again!
  * Crossover and Mutation okay
  * Population HAS NOT converged to global minimum with equivalent GA param to previous test!
    * Fitness=2589 Alleles=415.20,420.90

Small target could be giving a large fitness. How is the "model fit" evaluted?

* Smaller target: 0.1 and 0.01: Expect to get very high fitness
  * target 0.1: Fitness=16800 Alleles=422,422
  * target 0.01: Fitness=1570000 Alleles=422,422

So it seems like that for the *Schwefel_2D* CellML model at least, the *target* attribute seems to interact with fitness evaluation.

## 23.06.2015
### Selection
* [x] Print the new population *selected* before the *breeding stage*
  * should be less verbose than individual feedback of selection operator as it is a batch summary

### Task distribution
The *root* processor (rank=0) handles the majority of tasks in the Genetic Algorithm except for fitness evaluation.
Each genome well specifies a CellML model's parameters so that a simulation can be run with a CellML API.
The *fit* of the CellML model to the data points specified in the *virtual experiment* determines the corresponding genome's fitness.

Parallel processing is natural to such problem: assign a free processor a *genome* to evaluate the fitness for.

The *Distributor* class handles the task distribution and synchronisation.

Optimising *Distributor::process* for the number of processors, average simulation time, and number of tasks required, could be a worthwhile task.

## 24.06.2015
* [x] Process yesterday's todo list
  * [x] Selected population feedback before breeding
  * [x] Organise and document verbosity settings in README
  * [x] Update GAEngine.h to apply the new verbosity setting
    * [x] verbosity:0
	* [x] 1
	  * Could make format nicer
	* [x] 2
	  * Tagging each genome with [index] is slightly annoying
	* [x] 3
	  * The tagging is good for GO stage feedback
	* [x] 4	

## 25.06.2015
### CellML-VE fitness evaluation
#### Method

### TODO
* [x] Add comments to functions in VirtualExperiment
* [x] Add comments to functions in VEGroup

## 26.06.2015
### BUGS
Bugs in model evaluation method in *virtexp.cpp* have been addressed below:

* VirtualExperiment::Evaluate()
  * [ ] All simulation points sufficiently close to VE data points are selected for SSR evaluation (not strictly 1-1)
  * Algorithm to select simulation point is using EPSILON
    * [x] replace eps argument as m_Accuracy for calls to *in_range* function
  * [ ] Should check if simulation has given estimation for *all* time-points in VE (No time-point missed)

* GetSSRD
  * Should check if there is one-to-one relationship between experimental data (m_Timepoints) and estimates (input vector)
    * [ ] Check if the estimates vector and m_Timepoints are equal size
  * [ ] Normalisation method fails if experimental data is *zero*

## 27.06.2015
Process adressed bugs

## 29.06.2015
### Diagnosis
#### Unique estimation
Check to see if a VE data point is estimated by multiple simluation points AND multiply summed

* A simple error log

* Schwefel2D
Realised that there was only one target anyway and ODE was solving a dummy. Always a 1-1 relationship for experimental data-simulation estimation pair. Very good.
```
...
0:1
0:1
0:1
...
```

Rewrote the code to collect estimation vector to the VE data point vector. Fix is implemented when DEBUG_BUILD is not defined.

#### Sum of Squared Residuals
Check for equal size between simulation and experimental data vectors added.
INFINITY returned as SSR if the check fails.

getSSRD function expects ordering of data in the estimation vector (d) to be identical to m_Timepoints vector.
New implementation of estimation selector guarantees equivalent ordering **except** gaps in estimations. 
Hence, even before getSSRD is called, the **completeness** as well as **uniqueness** of estimation vector needs to be checked.

* [x] Uniqueness: two elements in estimation vector correspond to two different data points
  * guaranteed by the estimation selector algorithm

```c++
for(int i=0;i<m_Timepoints.size();i++)
{
	bool b_match=false;	// flag to indicate if a data point has been matched with an appropriate estimation
	// iterate the simulation points and get the first point in range of the data
	for(int j=0;j<vd.size();j+=recsize)
	{
		// check if sim-point is in range
		if (in_range(vd[j],m_Timepoints[i].first,m_Accuracy))
		{
			results.push_back(make_pair(i,vd[j+m_nResultColumn]));	// add the var of interest
			b_match=true;
			break;	// done with this data-point
		}
	}
	if(!b_match)
		std::cerr << "Error: Simulation cannot estimate VE data-point" << std::endl;
}
```

* [x] Complenetess: each element in experimental data vector has a corresponding estimation in estimation vector
  * Compare size of two vectors before call to getSSRD
  * return INF for mismatch in size

```c++
// FIX
// Check completeness of estimation points selected from simulation
res=((results.size()==m_Timepoints.size())?getSSRD(results):INFINITY);
```

### Optimisation
VEGroup::Evaluate returns a finite fitness value only when regression analysis for every VE is *successful*. 
Therefore, as soon as a single VE has been found to give error, the function should immediately return INFINITY rather than continuing to simulate the remaining VEs.

```c++
// update the total residual
if(d!=INFINITY)
{	
    res+=d;
    count++;
}
#ifdef DEBUG_BUILD
// If any experiment evaluated to INFINITY, this loop should immediately return INFINITY, since all experiments need to be processed properly
else
{
	fprintf(stderr,"Error in evaluating Experiment[%d] with parameters: ",i);
	v.print(stderr);	// print model parameters
	return INFINITY;
}
#endif
```

### TODO
* Other CellML models
  * Ramp_2D
    * [x] add to codebase
	* [ ] test
  * ip3model
    * [x] add to codebase
	* [ ] test
	  * All alleles INF!
	    * it was a bug in best estimation selector algorithm

### BUGS
* Assessment points should be in chronological order?!
#### ip3model
* Must run with 1 processor only to avoid multiple processors writing to the error log
  * set *ntasks* and *cpus-per-task* to 1

##### Error message
```
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
Error: Simulation cannot estimate VE data-point
0:0
1:0
2:0
3:0
4:0
5:0
6:0
7:1
Error in evaluating Experiment[0] with parameters: kf5->855.197337 kf4->838.399435 kf16->104.028944 Rpc->4127.398099
```

Let's probe into the simulation result to see how sparse the solution really is...

Indeed, the simulation is not sparse enough, but we could do with what we have..?

##### Best estimation
Find the index to the frame in simulation result vector closest in time to the VE data point.


### Meeting minutes
* Disallow user from supplying VE with zero target (cannot implement a satisfactory normalisation yet)
* Check for chronological ordering of assessment points in XML file
* Implement ReportStep in CellML API as default
  * GCD of all timepoints 
* Remove *Block Sampling* feature in GA

## 30.06.2015
### Job result
A relatively large job for ip3model was ran after fixing the small bug in best estimation finder. 
Strangely, the job suffered an error before walltime and was killed. Below is the job description and error log.

Upon checking some parameters tested by GA, the solver seemed to have been working correctly.

The error could have been due to memory leakage - could be from our codebase or the CellML ODE solver.

#### Job description
In the slurm file
```
#!/bin/bash
#SBATCH -J CellML_Test
#SBATCH -A uoa00322
#SBATCH --time=10:00:00     # Walltime
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=1024  # memory/cpu (in MB)
#SBATCH -o exper_cellml_%j.out       # OPTIONAL
#SBATCH -e exper_cellml_%j.err       # OPTIONAL
#SBATCH --mail-type=ALL
#SBATCH --mail-user=dk.shin1992@gmail.com
#SBATCH -C sb
######################################################

#module load intel/ics-2013
#module load impi
module load ictce/5.4.0

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/projects/uoa00322/mike.cooling/cellml-sdk/lib/
export LIBRARY_PATH=$LIBRARY_PATH:/projects/uoa00322/mike.cooling/cellml-sdk/lib/
srun /projects/uoa00322/david.shin/cellml-ga/experiment ip3model1.xml -v -v
```

#### Error log
In exper_cellml_14309850.err (error log)
```
srun: error: compute-b1-051: task 0: Killed
slurmstepd: error: *** JOB 14309850 CANCELLED AT 2015-06-30T04:08:07 DUE TO TIME LIMIT on compute-b1-051 ***
srun: Job step aborted: Waiting up to 32 seconds for job step to finish.
```

#### Test file
Comments: Range for parameters is far too large and unrealistic(?). Running a few parameter sets on OpenCOR hints that the upper bound for the below parameters may be far too large.
```xml
<?xml version="1.0"?>
<CellMLTimeSeriesFit>
        <GA InitialPopulation="100" Generations="1000" Mutation_proportion="0.20" Crossover_proportion="0.70">
                <Alleles>
                        <Allele Name="kf5" LowerBound="1.0e-8" UpperBound="9.999e2"/>
                        <Allele Name="kf4" LowerBound="1.0e-8" UpperBound="9.999e2"/>
                        <Allele Name="kf16" LowerBound="1.0e-8" UpperBound="9.999e2"/>
                        <Allele Name="Rpc" LowerBound="1.0e-2" UpperBound="5e3"/>
                </Alleles>
        </GA>
        <VirtualExperiments>
                <VirtualExperiment ModelFilePath="ip3model.cellml" ResultColumn="9">
                <!-- note this VE doesn't have any Parameters to set, but next VE does (ie optional Parameters entity here) -->
                        <AssessmentPoints>
                                <AssessmentPoint time="100.0" target="0.026761882" />
                                <AssessmentPoint time="200.0" target="0.032711469" />
                                <AssessmentPoint time="400.0" target="0.035444437" />
                                <AssessmentPoint time="650.0" target="0.034428839" />
                                <AssessmentPoint time="1000.0" target="0.032079332" />
                                <AssessmentPoint time="5000.0" target="0.018530385" />
                                <AssessmentPoint time="7500.0" target="0.016311098" />
                                <AssessmentPoint time="10000.0" target="0.015490316" />
                        </AssessmentPoints>
                </VirtualExperiment>
                <VirtualExperiment ModelFilePath="ip3model.cellml" ResultColumn="1">
                        <Parameters>
                                <Parameter ToSet="Ls" Value="5.0"/>
                        </Parameters>
                        <AssessmentPoints>
                                <AssessmentPoint time="100.0" target="0.1" />
                                <AssessmentPoint time="200.0" target="0.0957163" />
                                <AssessmentPoint time="400.0" target="0.09551297" />
                                <AssessmentPoint time="650.0" target="0.095987597" />
                                <AssessmentPoint time="1000.0" target="0.098171588" />
                                <AssessmentPoint time="5000.0" target="0.09932246" />
                                <AssessmentPoint time="7500.0" target="0.099750063" />
                                <AssessmentPoint time="10000.0" target="0.099907954" />
                        </AssessmentPoints>
                </VirtualExperiment>
        </VirtualExperiments>
</CellMLTimeSeriesFit>
```

### TODO Jobs from the meeting
* [x] Disallow user from supplying VE with zero target (cannot implement a satisfactory normalisation yet)
* [x] Check for chronological ordering of assessment points in XML file
* [ ] Implement ReportStep in CellML API as default
  * [ ] GCD of all timepoints 
* [x] Remove *Block Sampling* feature in GA

### Checking for invalid VEs
Added a member function in VirtualExperiment class *isValid* to check for a VE's validity.
Below is an error output of a Schwefel2D test with 0.0 as a target value.
The job used 4 tasks, 2 cpus.
```
Error in VirtualExperiment - zero is not allowed as target
Error in VirtualExperiment - zero is not allowed as target
Error in VirtualExperiment - zero is not allowed as target
Error in VirtualExperiment - zero is not allowed as target
srun: error: compute-gpu-d1-005: tasks 0-3: Exited with exit code 255
```

Reassigning the target to 0.1... and re-running the test, no error message is generated, and the test is successful.

## 01.07.2015
### Check chronological ordering of assessment points
Add a further condition to VirtualExperiment::isValid function to check for chronological ordering of assessment points.

A few minor advantages we see from checking chronological ordering are:
* Checks some user typos
* Readability of Virtual experiment data in XML file

Test file: test_order.xml (in 3-ip3model test directory) 
Output:
```
Error: VirtualExperiment: assessment points are not in chronological order
Error: VirtualExperiment: assessment points are not in chronological order
srun: error: compute-b1-015: task 0: Exited with exit code 255
srun: error: compute-b1-019: task 1: Exited with exit code 255
```

### Remove "Block Sampling" feature in GA
Done

### Report steps to CellML API as default
#### TODO
* Read CellML API documentation
 
### Clean code
Clean patched code from seqmode including preprocessor directives, unreferenced variables, and comments.

### TODO
* In GAEngine::select_weighted, a better way to calculate sum? Sum can OVERFLOW!? Even if not, the method is not simple.

## 02.07.2015
### RNG Seeding
A change has been made to how each processor's RNG is seeded.

Now the seed is dependent on *time* and *proc* (the processor rank).

### Clean-up best genome summary
Tidy-up the section in experiment.cpp summarising the best genome in GA, so that it is consistent in format with other genome reports

New style: (a test on 2D shifted Schwefel function)
```
==========================================
BEST GENOME (0.000060):
x1=920.826503  x2=920.769700
==========================================
```

### TODO
* GAEngine.h getting too large with details
  * [ ] Create GAEngine.cpp to handle function definitions

### New test
A sensible test VE data for  *ip3model* is added: ip3model-1.xml

Isn't IP3 the 10th variable? (time being the 0th)

* As a *simple* test, only ip3 will be *measured* by the VE
* Default CellML model param setting used
* Measurement taken from the OpenCOR simulation at constant time-intervals: 200s (Except at the end we do a slightly big jump)
* Searching 4 param space
  * kf5 = 0.0004
  * kf4 = 0.3 
  * kf16 = 1.25 
  * Rpc = 4.61 

From analysing the result of test, we see that a solution completely different to test solution return a very small fitness... Something is wrong

## 03.07.2015
### Diagnosing ip3model-1 test
Simplify the test further:
* timepoints: 0, 200, ..., 1000 sec
* Run on a single processor for comprehensible debugging

#### Check estimation vector
```
----------------------------------------
DEBUG:
m_Timepoints: (0,0.015), (200,0.03264), (400,0.03548), (600,0.03472), (800,0.03345), (1000,0.03209),
results: (0,0.015), (1,0.100309), (2,0.100309), (3,0.100309), (4,0.100309), (5,0.100309),
----------------------------------------
```

#### Check estimation times
Okay, so we are still missing what the sim-times are: add another debug message

```
<Assessment time,best_est>=<0.000000,0.000000>
<Assessment time,best_est>=<200.000000,200.165996>
<Assessment time,best_est>=<400.000000,400.165996>
<Assessment time,best_est>=<600.000000,600.165996>
<Assessment time,best_est>=<800.000000,800.165996>
<Assessment time,best_est>=<1000.000000,1000.000000>
----------------------------------------
DEBUG:
m_Timepoints: (0,0.015), (200,0.03264), (400,0.03548), (600,0.03472), (800,0.03345), (1000,0.03209),
results: (0,0.015), (1,0.0258055), (2,0.0258055), (3,0.0258055), (4,0.0258055), (5,0.0258055),
```

#### Check SSR calculation
The above output is just one example, and the *ip3* (?) values are *quite* close! For the sake of debugging, SSR is calculated:
```MATLAB
data = [0.015, 0.03264, 0.03548, 0.03472, 0.03345, 0.03209];
sim = [0.015, 0.0258055, 0.0258055, 0.0258055, 0.0258055, 0.0258055];

SSR = sum(abs((data-sim)./data).^2);
```

OUTPUT:
```
SSR =

    0.2747
```

#### Check matching genome (parameter set) to the above simulation
Which probably matches to the genome from GA output:
```
[30](0.274699) [92] kf5=21.250300   kf4=547.342355   kf16=932.391349   Rpc=3593.062508
```

While the best genome gives fitness (SSR):
```
==========================================
BEST GENOME (0.006986):
kf5=997.290339  kf4=547.987246  kf16=926.621998  Rpc=4632.879708
==========================================
```

#### Check against simulation on OpenCOR
The matching parameter-set should be simulated on OpenCOR to compare against the program's ODE solver results.
i.e. does **kf5=21.250300   kf4=547.342355   kf16=932.391349   Rpc=3593.062508** reproduce *Ip3* behaviour of result **(0,0.015), (1,0.0258055), (2,0.0258055), (3,0.0258055), (4,0.0258055), (5,0.0258055)**?

Yes. *Ip3* is initialised at 0.015 and quickly settles to 0.0258055. The behaviour overall resembles a step function which is definitely not what the test param-set gives - a rise and decay type.

#### Conclusion
The program is running as it should. To optimize the parameters further (and to get a sensible solution):
* Increase generation
* Increase population
* Narrower range (realistic)
* More assessment points

### Larger ip3model test
ip3model-1-lge.xml
* Encountered error at Gen 255

Error output:
```
Error evaluating model
Error in evaluating Experiment[0] with parameters: kf5=675.378570  kf4=281.417074  kf16=923.599894  Rpc=4632.879708
srun: error: compute-b1-039: task 0: Killed
srun: Job step aborted: Waiting up to 32 seconds for job step to finish.
slurmstepd: error: *** STEP 14366332.0 CANCELLED AT 2015-07-03T17:35:33 DUE TO TIME LIMIT on compute-b1-039 ***
slurmstepd: error: *** JOB 14366332 CANCELLED AT 2015-07-03T17:35:33 DUE TO TIME LIMIT on compute-b1-039 ***
```

* Population at Gen-255 is still far from the test solution

Check the param set with OpenCOR:
* Can solve, but almost a step function

### Narrow range ip3model test
ip3model-1-nrw.xml
* Encountered error at Gen 209

Error output:
```
slurmstepd: error: Step 14366338.0 exceeded memory limit (8485688 > 8388608), being killed
slurmstepd: error: Exceeded job memory limit
slurmstepd: error: *** STEP 14366338.0 CANCELLED AT 2015-07-03T13:20:09 *** on compute-c1-014
srun: Job step aborted: Waiting up to 32 seconds for job step to finish.
srun: error: compute-c1-014: tasks 0-3: Killed
```

Possible memory leakage in the program

### ip3model test with many assessment points: ip3model-1-many.xml
More assessment points will be provided, not necessarily at constant intervals, such that the ip3 curve can be better interpolated.

Total of 22 points have been selected from range 0-9395s.

Limit for parameters have been significantly narrowed, also.

#### Results
The test failed due to time-limit but the output until 992th generation has been logged.

The best chromosome in the 992th generation is:
```
[992](0.025468) [0] kf5=0.000507   kf4=0.251391   kf16=2.565132   Rpc=8.496967
```

Which is at least *graphically* a reasonable fit to the solution (run OpenCOR simulation).

## 06.07.2015
Merged the newtest branch onto patch

### CellML API: setTabulationStepControl setting

### Meeting minutes
* [x] Attach timestamps on error logs and output to make debugging easier
* [ ] User must report the timestep used in gathering experiment data; easier to set up solver
  * [x] Quit program if unspecified?
  * [x] Error message if unspecified
  * [ ] Check supplied ReportStep for consistency with data points
* [ ] Modify random selection of allele values to be a mantessa-exponent type 
* [ ] Organise the GAEngine code into source-header style
* [ ] Why haven't we seen **inf** genomes in the latest ip3model tests?
  * Force an INF and find out whether they are being handled properly
* [ ] Test the code with multiple experiments (multiple curve fitting)
* [ ] What makes a slurm job blow up?

### Timestamping error messages
Error message format:
```
Error: IN_LOCATION: REASON: TIMESTAMP 
```

Test output:
```
Error: VirtualExperiment::isValid: assessment points are not in chronological order: 2015-07-06.17:16:23
srun: error: compute-d1-055: task 0: Exited with exit code 255
```

### Timestamping output
* Per generation summary

## 07.07.2015

### Job outputs

#### 14449533
SLURM Job_id=14449533 Name=CellML_Test Failed, Run time 05:00:08, TIMEOUT

Output stops at 256th generation!

**exper_cellml_14449533.err**
```
Error: VirtualExperiment::Evaluate: error evaluating model: 2015-07-06.19:10:18
Error: VEGroup::Evaluate: error in evaluating Experiment[0] with parameters: kf5=8.918041  kf4=8.868596  kf16=5.695204  Rpc=26.014640
: 2015-07-06.19:10:18
srun: error: compute-d1-054: task 0: Killed
srun: Job step aborted: Waiting up to 32 seconds for job step to finish.
slurmstepd: error: *** JOB 14449533 CANCELLED AT 2015-07-06T23:42:22 DUE TO TIME LIMIT on compute-d1-054 ***
slurmstepd: error: *** STEP 14449533.0 CANCELLED AT 2015-07-06T23:42:22 DUE TO TIME LIMIT on compute-d1-054 ***
```

Timeout failure email (*SLURM Job_id=14449533 Name=CellML_Test Failed, Run time 05:00:08, TIMEOUT*) received at 11:42pm, which was much later than when the task was killed.

Conclusion: the reported runtime errors are **fatal** to the program

#### 14449566
SLURM Job_id=14449566 Name=CellML_Test Ended, Run time 01:27:44, COMPLETED, ExitCode 0

No error messages generated.

Example per-gen output:
```
2015-07-06.18:56:44
Generation 23. Best fitness: 0.426895
[0] kf5=0.001430   kf4=0.282692   kf16=6.074622   Rpc=19.165585
```

Best genome:
```
==========================================
BEST GENOME (0.009884):
kf5=0.000337  kf4=0.386681  kf16=1.935865  Rpc=6.215836
==========================================
```

### TODO
* [ ] Summarise at the start of the *.out* file the main GA settings
  * the job number doesn't explain much about what the run was about, esp. during debugging stages


### Report time-step
Enforce user to supply timestep used in gathering experimental data in each *VirtualExperiment* tag.

Note that the timestep is accessed at *VirtualExperiment::LoadExperiment* when the program - by all procs - is reading each VE data:
```c++
vx->m_ReportStep=atof(elem.GetAttribute("ReportStep").GetValue().c_str());
```

2 VEs both with ReportStep unspecified:
```
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - program will continue with default settings: 2015-07-07.11:29:43
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - program will continue with default settings: 2015-07-07.11:29:43
```

The current version's manner has been that user has the burden of supplying a **valid** VE dataset. For example, the program quits with an error message if it encounters a zero target in the XML file.
A similar behaviour is programmed if *ReportStep* is unspecfied. 

For this case, a new member (*b_Error*) has been added to the VirtualExperiment class to act as a flag that is raised when any other error is encountered during any stage of handling of a VE object, such as encountering an unspecified ReportStep during the loading stage.

VirtualExperiment::isValid is programmed to check b_Error, and it is up to the programmer to decide when/where to raise the flag.

#### Outputs
Two VE's: One with unspecified ReportStep:
```
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - program will continue with default settings: 2015-07-07.14:35:40
Error: VirtualExperiment::isValid: an error was encountered in this VirtualExperiment: 2015-07-07.14:35:40
srun: error: compute-b1-033: task 0: Exited with exit code 255
```

Two VE's: ReportSte: 0.0 and 50.0
```
DEBUG: ReportStep: 0
DEBUG: ReportStep: 50
```

#### Outputs v2
```
Error: VirtualExperiment::LoadExperiment: raising error flag: ReportStep is unspecified - set to 0.0 if unknown: 2015-07-07.14:50:10
Error: VirtualExperiment::isValid: an error was encountered in this VirtualExperiment: 2015-07-07.14:50:10
srun: error: compute-b1-033: task 0: Exited with exit code 255
```

#### Outputs v3
```
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - set to 0.0 if unknown: raising error flag: 2015-07-07.15:51:56
Error: VirtualExperiment::isValid: error flag is raised: 2015-07-07.15:51:56
srun: error: compute-e1-021: task 0: Exited with exit code 255
```

### Checking ReportStep for consistency with data points
Should be capable of working with floating precision times and ReportStep values.

Check how close each time-point is as a multiple of ReportStep.

#### Tests
##### No ReportStep
```
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - set to 0.0 if unknown: raising error flag: 2015-07-07.16:22:10
Error: VirtualExperiment::isValid: error flag is raised: 2015-07-07.16:22:10
srun: error: compute-b1-033: task 0: Exited with exit code 255
```
Okay good

##### 0.0 ReportStep
```
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
DEBUG HERE: 4
DEBUG HERE: 5
DEBUG HERE: 6
DEBUG HERE: 7
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
DEBUG HERE: 4
DEBUG HERE: 5
DEBUG HERE: 6
DEBUG HERE: 7
```
Uh oh... This is okay, but what we've missed is that when ReportStep is zero, we are *dividing time by zero* which gives a meaningless result. In fact, if user specifies zero ReportStep, we should skip the consistency check!

##### 0.0 ReportStep v2
```
DEBUGGING: user supplied 0.0 ReportStep
DEBUGGING: user supplied 0.0 ReportStep
```
Better! Now the program has skipped the check for ReportStep when it has been intentionally declared zero by user.

##### No ReportStep v2
```
Error: VirtualExperiment::LoadExperiment: ReportStep is unspecified - set to 0.0 if unknown: raising error flag: 2015-07-07.16:41:01
Error: VirtualExperiment::isValid: error flag is raised: 2015-07-07.16:41:01
srun: error: compute-b1-011: task 0: Exited with exit code 255
```
Good, still working

##### Non-zero inconsistent ReportStep
```
DEBUG HERE: 0
Error: VirtualExperiment::isValid: inconsistent ReportStep 10 with time 100: 2015-07-08.10:55:53
srun: error: compute-b1-061: task 0: Exited with exit code 255
```
BUG! THe inconsistency error is raised at the wrong place.

CODE:
```c++
for(int i=0;i<m_Timepoints.size();i++)
		{
			std::cerr << "DEBUG HERE: " << i << "\n";

			double n=m_Timepoints[i].first/m_ReportStep;
			if(fabs(n-round(n))<0.1)
			{
				std::cerr << "Error: VirtualExperiment::isValid: inconsistent ReportStep " << m_ReportStep << " with time " << m_Timepoints[i].first << ": " << currentDateTime() << std::endl;
				return false;
			}
		}
```
Oops! the bug is at the conditional *if(fabs(n-round(n))<0.1)*: Wrong comparator. Should check if |n-round(n)| **>** 0.1

##### Non-zero inconsistent ReportStep v2
```
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
DEBUG HERE: 4
DEBUG HERE: 5
DEBUG HERE: 6
DEBUG HERE: 7
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
Error: VirtualExperiment::isValid: inconsistent ReportStep 100 with time 650: 2015-07-08.11:30:17
srun: error: compute-b1-032: task 0: Exited with exit code 255
```
Correctly, consistency checking passes the first VE, for which the reportstep was 10.0 s and all time points were exactly integer multiples of it.
The exception case in the second VE was spotted by the program, the 4th AssessmentPoint with time 650, not an int multiple of supplied RepStep 100s.

A comprehensible error message too.

##### Consistent ReportStep
```
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
DEBUG HERE: 4
DEBUG HERE: 5
DEBUG HERE: 6
DEBUG HERE: 7
DEBUG HERE: 0
DEBUG HERE: 1
DEBUG HERE: 2
DEBUG HERE: 3
DEBUG HERE: 4
DEBUG HERE: 5
DEBUG HERE: 6
DEBUG HERE: 7
```
Very good, all times were checked and passed.

##### 0.0 ReportStep v3
Add a warning message when user leaves report step from ODE solver to default (refer to CellML API).

```
Warning: VirtualExperiment::isValid: user has supplied 0.0 ReportStep: ODE solver will automatically determine steps and cannot guarantee accurate regression analysis: 2015-07-08.11:57:06
```
Okay.

