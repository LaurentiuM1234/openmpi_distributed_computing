#ifndef _COORDINATOR_H_
#define _COORDINATOR_H_

#include <mpi.h>

#include "arraylist.h"

struct coordinator {
	arraylist_t workers_; // list of connected worker nodes
	arraylist_t coordinators_; // list of connected coordinator nodes

	int rank_; // rank of coordinator (same rank for all used communicators)
	int world_node_count_; // number of nodes in the world communicator
	int coord_node_count_; // number of nodes in the coord communicator

	MPI_Comm coord_comm_; // communicator used by coordinator nodes

	int **topology_; // matrix representing the topology of the system

        int leader_; // leader of the topology
        int *parents_; // array of parents for all coordinator nodes

	int array_size_; // size of the array used in the task stage
	int **work_matrix_; // matrix used to distribute work in the task stage
};

/**
 * @brief Run the coordinator node tasks
 * 
 * @param array_size size of the array used in the task stage 
 * @param has_error has link error between nodes 0 and 1
 * @return -ELIB - if any of the system calls failed
 *         -EREAD - if read operation fails
 * 	   0 - upon success
 */
int run_coordinator(int array_size, int has_error);

#endif // _COORDINATOR_H_