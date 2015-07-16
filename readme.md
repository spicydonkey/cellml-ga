# CellML-GA project

## Summary
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


```
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
```

Performance improvement (population: 100, 50 generations):

Cores  Runtime, min
1       70:00
8       09:51
16      05:26
32      02:41
64      01:32
128	00:59

Note:

## How to use
### Required packages
#### Required compiler:
* mpiicpc: an Intel MPI C++ compiler to compile the codebase. Load onto Pan Cluster's **build node** by:
```
module load intel/ics2013
``` 
	Note: You must be logged onto one of NeSI's "build-nodes" to load a module and compiling

#### Required libraries:
* CellML-API
* AdvXMLParser

### Before you compile
The first thing you should do is to configure the makefile:

Configure *PROJ_PATH* to point to the project directory in which the *AdvXMLParser* library is located.
e.g.
```
PROJ_PATH=/projects/uoa00322/your_personal_project_dir
```

Configure *CELLML_PATH* to point to the installed CellML-API directory.
e.g.
```
CELLML_PATH=/projects/uoa00322/your_personal_project_dir/CELLML_API_DIRECTORY
```


