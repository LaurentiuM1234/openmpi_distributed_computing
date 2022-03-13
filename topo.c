#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "topo.h"
#include "error.h"
#include "util.h"


static int announce_topology(int **topology, int rows, int columns, int rank)
{
	printf("%d -> ", rank);

        for (int i = 0; i < rows; i++) {
                int last_rank;

                // iterate through the current row in order to find the rank
                // of the last reachable node
                for (int j = 0; j < columns; j++) {
                        if (topology[i][j] == 1) {
                                last_rank = j;
                        }
                }

                printf("%d:", i);
                for (int j = 0; j < columns; j++) {
                        if (topology[i][j] == 1) {
                                if (j != last_rank) {
                                        printf("%d,", j);
                                } else {
                                        printf("%d", j);
                                }
                        }
                }
                printf(" ");
        }
        printf("\n");

        fflush(stdout);

        return 0;
}


static int compute_spanning_tree(struct coordinator *coord)
{
	int ret;
	int neigh_count = get_size(coord->coordinators_);

	coord->parents_ = calloc(coord->coord_node_count_, sizeof(int));

	if (coord->parents_ == NULL)
		return -ELIB;

	int *recv_dummy = calloc(coord->coord_node_count_, sizeof(int));

	if (recv_dummy == NULL) {
		ret = -ELIB;
		goto err_free_parents;
	}

	// initially all the nodes have no parents
	memset(coord->parents_, -1, sizeof(int) * coord->coord_node_count_);
	memset(recv_dummy, -1, sizeof(int) * coord->coord_node_count_);

	if (coord->rank_ != coord->leader_) {
		MPI_Status status;

		// wait for message from parent
		MPI_Recv(recv_dummy, coord->coord_node_count_,
				MPI_INT, MPI_ANY_SOURCE, 0,
				coord->coord_comm_, &status);

		// received message from parrent, set value in
		// parents array
		coord->parents_[coord->rank_] = status.MPI_SOURCE;
	}

	for (int i = 0; i < neigh_count; i++) {
		int crt_rank = *(int*)get(coord->coordinators_, i);

		if (crt_rank != coord->parents_[coord->rank_]) {
			// log message before sending
			printf("M(%d,%d)\n", coord->rank_, crt_rank);
			fflush(stdout);

			// send vector of parents to neighbours
			// (except the parent node)
			MPI_Send(coord->parents_, coord->coord_node_count_,
					MPI_INT, crt_rank, 0, 
					coord->coord_comm_);
		}
	}

	for (int i = 0; i < neigh_count; i++) {
		int crt_rank = *(int*)get(coord->coordinators_, i);

		if (crt_rank != coord->parents_[coord->rank_]) {
			// receive vector of parents from neighbours
			// (except the parent)
			MPI_Recv(recv_dummy, coord->coord_node_count_,
					MPI_INT, crt_rank, 0, 
					coord->coord_comm_, NULL);
			
			// update vector of parents based on received
			// information
			for (int j = 0; j < coord->coord_node_count_; j++) {
				if (recv_dummy[j] != -1) {
					coord->parents_[j] = recv_dummy[j];
				}
			}
		}
	}

	// any coordinator that's not the leader will send the vector
	// of parents to its parent and receive the vector of parents
	// from its parent
	if (coord->rank_ != coord->leader_) {
		// log message before sending
		printf("M(%d,%d)\n", coord->rank_,
			coord->parents_[coord->rank_]);
		fflush(stdout);

		MPI_Send(coord->parents_, coord->coord_node_count_, MPI_INT,
				coord->parents_[coord->rank_], 0,
				coord->coord_comm_);

		MPI_Recv(coord->parents_, coord->coord_node_count_, MPI_INT,
				coord->parents_[coord->rank_], 0, 
				coord->coord_comm_, NULL);
	}

	// send vector of parents to children
	for (int i = 0; i < coord->coord_node_count_; i++) {
		if (coord->rank_ == coord->parents_[i]) {
			// log message before sending
			printf("M(%d,%d)\n", coord->rank_, i);
			fflush(stdout);

			MPI_Send(coord->parents_, coord->coord_node_count_,
					MPI_INT, i, 0, coord->coord_comm_);
		}
	}

	return 0;

err_free_parents:
	free(coord->parents_);
	return ret;
}


static int build_topology_matrix(struct coordinator *coord)
{
	int **recv_dummy = NULL;

	int ret = create_matrix(&coord->topology_,
				coord->coord_node_count_,
				coord->world_node_count_);

	if (ret < 0)
		return ret;

	ret = create_matrix(&recv_dummy,
				coord->coord_node_count_,
				coord->world_node_count_);

	if (ret < 0)
		goto err_free_tmatrix;

	// mark all neighbours as traversable for current node
	for (int i = 0; i < get_size(coord->workers_); i++) {
		int crt_rank = *(int*)get(coord->workers_, i);

		coord->topology_[coord->rank_][crt_rank] = 1;
	}

	// receive information from all children and update the
	// topology matrix based on that information
	for (int i = 0; i < coord->coord_node_count_; i++) {
		if (coord->rank_ == coord->parents_[i]) {
			for (int j = 0; j < coord->coord_node_count_; j++) {
				MPI_Recv(recv_dummy[j],
					coord->world_node_count_,
					MPI_INT, i, 0,
					coord->coord_comm_, NULL);

				// skip vector corresponding to current coord's
				// rank because it doesn't hold valid information
				if (j == coord->rank_)
					continue;

				for (int k = 0; k < coord->world_node_count_;
					k++) {
					if (coord->topology_[j][k] == 0) {
						coord->topology_[j][k]
							= recv_dummy[j][k];
					}
				}
			}
		}
	}

	if (coord->rank_ != coord->leader_) {
		// send topology to parent
		for (int i = 0; i < coord->coord_node_count_; i++) {
			// log message before sending
			printf("M(%d,%d)\n", coord->rank_,
				coord->parents_[coord->rank_]);
			fflush(stdout);

			MPI_Send(coord->topology_[i], coord->world_node_count_,
				MPI_INT, coord->parents_[coord->rank_], 0,
				coord->coord_comm_);
		}
	
		for (int i = 0; i < coord->coord_node_count_; i++) {
			// wait for topology from parent
			MPI_Recv(recv_dummy[i], coord->world_node_count_,
					MPI_INT, coord->parents_[coord->rank_],
					0, coord->coord_comm_, NULL);

			if (i == coord->rank_)
				continue;

			for (int j = 0; j < coord->world_node_count_; j++) {
				if (coord->topology_[i][j] == 0) {
					coord->topology_[i][j]
						= recv_dummy[i][j];
				}
			}
		}
	}

	// send topology to children
	for (int i = 0; i < coord->coord_node_count_; i++) {
		if (coord->rank_ == coord->parents_[i]) {
			for (int j = 0; j < coord->coord_node_count_; j++) {
				// log message before sending
				printf("M(%d,%d)\n", coord->rank_, i);
				fflush(stdout);

				MPI_Send(coord->topology_[j],
						coord->world_node_count_,
						MPI_INT, i, 0, coord->coord_comm_);
			}
		}
	}

	return 0;
err_free_tmatrix:
	free_matrix(coord->topology_, coord->coord_node_count_);
	return ret;
}

static int establish_topology(struct coordinator *coord)
{
	int ret = compute_spanning_tree(coord);

	if (ret < 0)
		return ret;

	// wait for all coordinator nodes to have finished
	MPI_Barrier(coord->coord_comm_);

	ret = build_topology_matrix(coord);

	if (ret < 0)
		return ret;

	return 0;
}

static void propagate_topology(struct coordinator *coord)
{
	// send the topology matrix to each worker the coordinator
	// is connected to
	for (int i = 0; i < get_size(coord->workers_); i++) {
		int crt_rank = *(int*)get(coord->workers_, i);
		for (int j = 0; j < coord->coord_node_count_; j++) {
			// log message before sending
			printf("M(%d,%d)\n", coord->rank_, crt_rank);
			fflush(stdout);

			MPI_Send(coord->topology_[j], coord->world_node_count_,
					MPI_INT, crt_rank, 0, MPI_COMM_WORLD);
		}
	}
}


int handle_cside_topology(struct coordinator *coord)
{
	int ret = establish_topology(coord);

	if (ret < 0)
		return ret;

	// wait for all coordinator nodes to have finished
	MPI_Barrier(coord->coord_comm_);

	ret = announce_topology(coord->topology_,
                                coord->coord_node_count_,
                                coord->world_node_count_,
                                coord->rank_);

	if (ret < 0)
		return ret;

	// propagate topology to workers
	propagate_topology(coord);

	return 0;
}


static int receive_topology(struct worker *worker, int ***topology)
{
        MPI_Status status;

        int ret = create_matrix(topology,
                                worker->coord_node_count_,
                                worker->world_node_count_);

        if (ret < 0)
                return ret;

        for (int i = 0; i < worker->coord_node_count_; i++) {
                MPI_Recv((*topology)[i], worker->world_node_count_,
                        MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                        &status);
        }

        // mark worker's coordinator
        worker->coordinator_ = status.MPI_SOURCE;

        return 0;
}

int handle_wside_topology(struct worker *worker)
{
        int **topology;

        int ret = receive_topology(worker, &topology);

        if (ret < 0)
                return ret;
        
        ret = announce_topology(topology,
                                worker->coord_node_count_,
                                worker->world_node_count_,
                                worker->rank_);

        if (ret < 0) {
                free_matrix(topology, worker->coord_node_count_);
                return ret;
        }

        return 0;
}