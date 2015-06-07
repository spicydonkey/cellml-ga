
Porting of the original CellML GA-based distributed code to verify the GA.

Workitem distribution using MPI has been ported to work with a sequential process
scheduler. CellML simulations have been replaced by a test function called
similarly from the virtual experiment group object ultimately to evaluate
genome's fitness.


=================================
Test functions
=================================
Available test functions to serve as virtual CellML-VE fitting problem are:

+-----------------------+-------------------------------------------------------+
|	TestFunction	|			Description			|
+-----------------------+-------------------------------------------------------+
|	schwefel	| Schwefel function: min (420.9687,...) [-500,500]	|
|	sh_schwefel	| shifted Schwefel:	min (920.9687,...) [0,1000]	|
|	inf_schwefel	| Schwefel with [0,50] of INFINITY			|
|	neg_schwefel	| Schwefel shifted by -100 to give negative range	|
|	lge_schwefel		| Schwefel with [0,50] of very large value NOT INF		|
+-----------------------+-------------------------------------------------------+

Process
(1)	The above functions are defined in GATESTER.h following behaviour:
		double __TestFunction__(std::vector<double>);
(2)	Each test function is mapped to a corresponding test number in experiment.cpp:
		testFunctions[__testN__]=N;
(3)	Fitness evaluation VEGroup::Evaluate in virtexp.cpp calls corresponding test:
		case N:	return __testN__(params);
(4)	make the experiment with newly applied changes
		Pan Cluster:
		make -f makefile (or just make)

*IMPORTANT*
If you wish to implement this GA program with another test function or simulation, 
you must follow the steps outlined in Process carefully and add your code accordingly. 


ORIGINAL README.TXT
#####################################################################################################
CellML GA-based distrubuted code uses MPI to distribute the load across muliple computational nodes.

Load distribution is based on the workitems list. The list is populated with the item which contain
data to be computed and an opaque key, which is the context of the requestor. Once the list is fully
populated, call to process runs through the list requesting compute done on the data by sending the 
requests to the rest of the MPI ranks. Once the results are back, the observer function is called for
each result passing the workitem and the result to the callback.

Since CellML compiler relies on GCC compiler to build the code, LIBRARY_PATH and LD_LIBRARY_PATH 
should be configured by pointing to CellML library directory.

The example of Slurm job description file is given below (note the use of modules - the code in
the example below was built with Intel compiler and Intel MPI)
#####################################################################################################


#!/bin/bash
#SBATCH -J CellML_Test
#SBATCH -A uoa99999
#SBATCH --time=00:30:00     # Walltime
#SBATCH --ntasks=64
#SBATCH --mem-per-cpu=4096  # memory/cpu (in MB)
#SBATCH -o exper_cellml.out       # OPTIONAL
#SBATCH -e exper_cellml.err       # OPTIONAL
#SBATCH --mail-type=ALL
#SBATCH --mail-user=g.soudlenkov@auckland.ac.nz
#SBATCH -C sb
######################################################

module load intel/ics-2013
module load impi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/gsou008/work/UoA/mike.cooling/cellml-sdk/lib/
export LIBRARY_PATH=$LIBRARY_PATH:/home/gsou008/work/UoA/mike.cooling/cellml-sdk/lib/
srun ./experiment xp.xml


Performance improvement (population: 100, 50 generations):

Cores  Runtime, min
1       70:00
8       09:51
16      05:26
32      02:41
64      01:32
128	00:59


Note: The GA code may still support block sampling if experiment.cpp file has
#define SUPPORT_BLOCK_SAMPLING line uncommented


