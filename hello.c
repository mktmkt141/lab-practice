#include <stdio.h>
#include <mpi.h>
#include <unistd.h> // gethostname 関数を使うために必要

int main(int argc, char **argv){
    int rank, size;
    char hostname[256]; // ホスト名を格納する配列

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ホスト名を取得
    gethostname(hostname, sizeof(hostname));
    hostname[sizeof(hostname) - 1] = '\0'; // Null終端を保証

    printf("Hello World from %s, I am rank %d of %d\n", hostname, rank, size);

    MPI_Finalize();
    return(0);
}
