#!/bin/bash
#SBATCH -J CellML_Test
#SBATCH -A uoa00322
#SBATCH --time=00:30:00     # Walltime
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=2
#SBATCH --mem-per-cpu=1024  # memory/cpu (in MB)
#SBATCH -o exper_cellml_%j.out       # OPTIONAL
#SBATCH -e exper_cellml_%j.err       # OPTIONAL
#SBATCH -C sb
######################################################

module load impi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/PATH/TO/CELLML-API/cellml-sdk/lib/
export LIBRARY_PATH=$LIBRARY_PATH:/PATH/TO/CELLML-API/cellml-sdk/lib/
srun ../../experiment test.xml -v -v 
