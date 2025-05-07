#include <mpi.h>
#include <stdio.h>

#define N  1000

int main(int argc, char* argv[]) {
    // MPI環境の初期化
    MPI_Init(&argc, &argv);

    // ランク（各プロセスに付けられた一意な番号）とプロセス数の取得
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // プロセス0がプロセス数と全体の計算範囲を表示
    if (rank == 0) {
        printf("[プロセス0] %d個のプロセスで協力して1から%dまでの和を求めます！\n", size, N);
    }

    // 各プロセスが計算する範囲を決定
    int start = rank * N / size + 1;
    int end = (rank + 1) * N / size;

    // 各プロセスがそれぞれの計算範囲を表示
    printf("[プロセス%d] %dから%dまでの和を計算します...\n", rank, start, end);

    // 部分和の計算
    int local_sum = 0;
    for (int i = start; i <= end; i++) {
        local_sum += i;
    }

    // 各プロセスがそれぞれの結果を表示
    printf("[プロセス%d] %dから%dまでの和は %d でした！\n", rank, start, end, local_sum);

    // 部分和を集約（プロセス0に合計を集める）
    int global_sum = 0;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // プロセス0が最終的な結果を表示
    if (rank == 0) {
        printf("[プロセス0] 1から%dまでの和は %d でした！\n", N, global_sum);
    }

    // MPI環境の終了
    MPI_Finalize();
}
