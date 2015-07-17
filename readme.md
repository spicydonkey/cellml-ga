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
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=4096  # memory/cpu (in MB)
#SBATCH -o exper_cellml_%j.out		# OPTIONAL
#SBATCH -e exper_cellml_%j.err		# OPTIONAL
#SBATCH --mail-type=ALL
#SBATCH --mail-user=user.email@auckland.ac.nz	# Email alerts
#SBATCH -C sb
######################################################

module load intel/ics-2013

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/PATH/TO/YOUR/cellml-sdk/lib/
export LIBRARY_PATH=$LIBRARY_PATH:/PATH/TO/YOUR/cellml-sdk/lib/

srun PATH/TO/YOUR/experiment TEST_FILE [-v [...]]
```

Performance improvement (population: 100, 50 generations):

Cores  Runtime, min
1       70:00
8       09:51
16      05:26
32      02:41
64      01:32
128	00:59


## How to use
### Required packages
#### Required compiler:
* mpiicpc: an Intel MPI C++ compiler to compile the codebase. Load onto Pan Cluster's **build node** by:
```
module load intel/ics-2013
``` 

**Note**: You must be logged onto one of NeSI's "build-nodes" to load a module and compiling

#### Required libraries:
* CellML-API
* AdvXMLParser

---

### Before you compile
The first thing you should do is to configure the makefile:

Configure *XMLPARSER_PATH* to point directly to the **AdvXMLParser** directory. 

Example:
```
XMLPARSER_PATH=/gpfs1m/projects/uoa00322/mike.cooling/AdvXMLParser
```

Configure *CELLML_PATH* to point directly to the **CellML-API** directory.

Example:
```
CELLML_PATH=/gpfs1m/projects/uoa00322/mike.cooling/cellml-sdk
```

---

### Compile
Compile the project by running the makefile:
```
make
```
which should generate the binary **experiment**

### Testing
Test the program by running a short run of the ip3model problem:

```
cd tests/3-ip3model
```

Edit the exemplar slurm file - short.sl.

Since CellML compiler relies on GCC compiler to build the code, LIBRARY_PATH and LD_LIBRARY_PATH 
should be configured by pointing to CellML library directory.

Finally, batch the slurm job file, which runs the ip3-short.xml test on a **single processor**.
```
sbatch short.sl
```

**NOTE**: when submitting a job using MPI library to NeSI, you must import a suitable library e.g. **ictce/5.4.0**/**impi**/**intel/ics-2013** at the Slurm job. The exemplar job description above shows this.
