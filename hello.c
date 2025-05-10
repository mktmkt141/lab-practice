#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv){
    int rank, size;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &name_len);  // ← 追加

    printf("Hello World, I am %d of %d running on %s\n", rank, size, processor_name);

    MPI_Finalize();
    return 0;
}
