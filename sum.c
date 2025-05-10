#include <mpi.h>
#include <stdio.h>

#define N  1000

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &name_len);  // ← ホスト名を取得！

    if (rank == 0) {
        printf("[プロセス%d - %s] %d個のプロセスで協力して1から%dまでの和を求めます！\n",
               rank, processor_name, size, N);
    }

    int start = rank * N / size + 1;
    int end = (rank + 1) * N / size;

    printf("[プロセス%d - %s] %dから%dまでの和を計算します...\n",
           rank, processor_name, start, end);

    int local_sum = 0;
    for (int i = start; i <= end; i++) {
        local_sum += i;
    }

    printf("[プロセス%d - %s] %dから%dまでの和は %d でした！\n",
           rank, processor_name, start, end, local_sum);

    int global_sum = 0;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("[プロセス%d - %s] 1から%dまでの和は %d でした！\n",
               rank, processor_name, N, global_sum);
    }

    MPI_Finalize();
}
