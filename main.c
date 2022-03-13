#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

#include "coordinator.h"
#include "worker.h"
#include "error.h"

int main(int argc, char *argv[])
{
	int rank;

	MPI_Init(&argc, &argv);

	// get node rank
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank < 3) {
		int ret = run_coordinator(atoi(argv[1]), atoi(argv[2]));

		if (ret < 0) {
			printerr("run_coordinator", ret);
			exit(EXIT_FAILURE);
		}
	} else {
		int ret = run_worker();

		if (ret < 0) {
			printerr("run_worker", ret);
			exit(EXIT_FAILURE);
		}
	}

	MPI_Finalize();

        return 0;
}
