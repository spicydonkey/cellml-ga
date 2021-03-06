#!/bin/bash
#SBATCH -J CellML_Test
#SBATCH -A uoa00322
#SBATCH --time=02:00:00     # Walltime
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=4096  # memory/cpu (in MB)
#SBATCH -o exper_cellml_%j.out       # OPTIONAL
#SBATCH -e exper_cellml_%j.err       # OPTIONAL
#SBATCH -C sb
######################################################

module load intel/ics-2013 

# User-configured paths
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:
export LIBRARY_PATH=$LIBRARY_PATH:

srun ../../experiment large.xml -v -v 
