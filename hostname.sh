#!/bin/bash
#SBATCH -p debug  # 自分のパーティションの名前
#SBATCH --job-name="hostname-test"
#SBATCH --nodes=4
#SBATCH --ntasks=4
srun hostname
