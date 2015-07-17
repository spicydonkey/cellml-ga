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


## Where to start?
### Required packages
#### Compiler
* ***mpiicpc***: an Intel MPI C++ compiler to compile the codebase. Load onto Pan Cluster's *build node* by:
```
module load intel/ics-2013
``` 

**Note**: You must be logged onto one of NeSI's "**build-nodes**" to load module and build projects

#### Libraries
* CellML-API
* AdvXMLParser

---

### Before you compile
The first thing you should do is to configure the include paths in **makefile**: *XMLPARSER_PATH* and *CELLML_PATH*.

Set *XMLPARSER_PATH* to point directly to the **AdvXMLParser** directory. 

Example:
```
XMLPARSER_PATH=/projects/uoa00322/mike.cooling/AdvXMLParser
```

Configure *CELLML_PATH* to point directly to the **CellML-API** directory.

Example:
```
CELLML_PATH=/projects/uoa00322/mike.cooling/cellml-sdk
```

---

### Ready to compile
Compile the project with the *makefile*:
```
make
```
which should generate the binary **experiment** in your current directory.

---

### Testing
Test the program with some supplied problems in the *tests* sub-directory (No examples for 2D-Ramp model has been supplied):

Jump into any available test folder: 

Example: IP3 model
```
cd tests/3-ip3model
```

Feel free to look into the example test (XML) and job (Slurm) files.

Before batching any job with the example Slurm files, make sure you configure the library paths!

Since CellML compiler relies on GCC compiler to build the code, LIBRARY_PATH and LD_LIBRARY_PATH 
should be configured by pointing to CellML **library** directory. For example:

```
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/projects/uoa00322/mike.cooling/cellml-sdk/lib/
LIBRARY_PATH=$LIBRARY_PATH:/projects/uoa00322/mike.cooling/cellml-sdk/lib/
```

Finally, batch the slurm job file! As an example, running `sbatch short.sl` will batch the example test *short.xml* on a single processor.

---

**NOTE**: when submitting a job using MPI library to NeSI, you must import a suitable library e.g. **ictce/5.4.0**/**impi**/**intel/ics-2013** at the Slurm job. The exemplar job description above shows this.

---

## Creating your own 'Virtual Experiment'
Quickest way is by looking at an example: adapted from *short.xml* in IP3model problem...
```
<?xml version="1.0"?>
<CellMLTimeSeriesFit>
        <GA InitialPopulation="10" Generations="10" Mutation_proportion="0.4" Crossover_proportion="0.30" RNG="1">
                <Alleles>
                        <Allele Name="kf5" LowerBound="1.0e-8" UpperBound="9.999e2"/>
                        <Allele Name="kf4" LowerBound="1.0e-8" UpperBound="9.999e2"/>
                        ...
                        <Allele Name="Rpc" LowerBound="1.0e-2" UpperBound="5e3"/>
                </Alleles>
        </GA>
        <VirtualExperiments>
                <VirtualExperiment ModelFilePath="ip3model.cellml" Variable="IP3" ReportStep="50.0">
                <!-- note this VE doesn't have any Parameters to set, but next VE does (ie optional Parameters entity here) -->
                        <AssessmentPoints>
                                <AssessmentPoint time="100.0" target="0.026761882" />
                                <AssessmentPoint time="200.0" target="0.032711469" />
                                ...
                                <AssessmentPoint time="10000.0" target="0.015490316" />
                        </AssessmentPoints>
                </VirtualExperiment>
                <VirtualExperiment ModelFilePath="ip3model.cellml" Variable="Ca" ReportStep="50.0">
                        <Parameters>
                                <Parameter ToSet="Ls" Value="5.0"/>
                        </Parameters>
                        <AssessmentPoints>
                                <AssessmentPoint time="100.0" target="0.1" />
                                ...
                                <AssessmentPoint time="10000.0" target="0.099907954" />
                        </AssessmentPoints>
                </VirtualExperiment>
                <VirtualExperiment>
                        ...
                </VirtualExperiment>
                ...
                <VirtualExperiment>
                        ...
                </VirtualExperiment>
        </VirtualExperiments>
</CellMLTimeSeriesFit>
```
