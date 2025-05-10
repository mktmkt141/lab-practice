#!/bin/bash
#SBATCH -p debug  # 自分のパーティションの名前
#SBATCH --job-name="mpi-test"
#SBATCH --nodes=4
#SBATCH --ntasks=4
srun <MPIプログラムの実行ファイルのパス>


//mpiの課題のhello.cをこのシェルスクリプトを用いて実行し、*.outファイルに期待する出力があるかを確認する。
