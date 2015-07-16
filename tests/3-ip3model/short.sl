#!/bin/bash
#SBATCH -J CellML_Test
#SBATCH -A uoa00322
#SBATCH --time=00:05:00     # Walltime
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=1024  # memory/cpu (in MB)
#SBATCH -o exper_cellml_%j.out       # OPTIONAL
#SBATCH -e exper_cellml_%j.err       # OPTIONAL
#SBATCH -C sb
######################################################

module load ictce/5.4.0

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:
#export LIBRARY_PATH=$LIBRARY_PATH:

srun ../../experiment ip3-short.xml -v -v 
