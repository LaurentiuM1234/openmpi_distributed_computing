#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coordinator.h"
#include "error.h"
#include "topo.h"
#include "util.h"
#include "task.h"

#define PFILE_SIZE 15
#define COORD_COLOR 0

static int add_workers(struct coordinator *coord)
{
	// extract path to file containing worker ranks
	char pathfile[PFILE_SIZE];
	sprintf(pathfile, "./cluster%d.txt", coord->rank_);

	FILE* fp = fopen(pathfile, "r");

	if (fp == NULL)
		return -ELIB;

	// read number of workers in file
	int count;
	int ret = fscanf(fp, "%d", &count);

	if (ret != 1) {
		ret = -ELIB;
		goto err_fclose;
	}

	for (int i = 0, crt_rank; i < count; i++) {
		// read current worker rank
		ret = fscanf(fp, "%d", &crt_rank);

		if (ret != 1) {
			ret = -ELIB;
			goto err_fclose;
		}
		
		// add current worker rank to list
		ret = push_back(coord->workers_, &crt_rank);

		if (ret < 0)
			goto err_fclose;
	}

	fclose(fp);
	return 0;

err_fclose:
	fclose(fp);
	return ret;
}

static int add_coordinators(struct coordinator *coord, int has_error)
{
	int ret;

	if (coord->rank_ == 0) {
		int c1 = 1;
		int c2 = 2;

		ret = push_back(coord->coordinators_, &c2);

		if (ret < 0)
			return ret;

		if (has_error == 0) {
			ret = push_back(coord->coordinators_, &c1);

			if (ret < 0)
				return ret;
		}
			
	} else if (coord->rank_ == 1) {
		int c1 = 0;
		int c2 = 2;

		ret = push_back(coord->coordinators_, &c2);

		if (ret < 0)
			return ret;

		if (has_error == 0) {
			ret = push_back(coord->coordinators_, &c1);

			if (ret < 0)
				return ret;
		}
	} else {
		int c1 = 0;
		int c2 = 1;

		ret = push_back(coord->coordinators_, &c1);

		if (ret < 0)
			return ret;

		ret = push_back(coord->coordinators_, &c2);

		if (ret < 0)
			return ret;
	}

	return 0;
}

static int add_node_info(struct coordinator *coord)
{
	// extract rank of coordinator using world communicator
	MPI_Comm_rank(MPI_COMM_WORLD, &coord->rank_);

	// extract number of nodes in world communicator
	MPI_Comm_size(MPI_COMM_WORLD, &coord->world_node_count_);

	// create new communicator used only by the coordinators
	MPI_Comm_split(MPI_COMM_WORLD,
			COORD_COLOR,
			coord->rank_,
			&coord->coord_comm_);

	// extract number of nodes in coordinator communicator
	MPI_Comm_size(coord->coord_comm_, &coord->coord_node_count_);
	
	return 0;
}

static int create_coordinator(struct coordinator **coord, int has_error,
				int array_size)
{
	*coord = calloc(1, sizeof(struct coordinator));

	if (*coord == NULL)
		return -ELIB;
	
	(*coord)->array_size_ = array_size;

	int ret = add_node_info(*coord);

	if (ret < 0)
		goto err_free_coord;

	ret = create_arraylist(&(*coord)->workers_, sizeof(int));

	if (ret < 0)
		goto err_free_coord;

	// add the workers the coordinator is in charge of
	ret = add_workers(*coord);

	if (ret < 0)
		goto err_free_workers;

	ret = create_arraylist(&(*coord)->coordinators_, sizeof(int));

	if (ret < 0)
		goto err_free_workers;

	// add the other coordinators the current one can communicate with
	ret = add_coordinators(*coord, has_error);

	if (ret < 0)
		goto err_free_coordinators;

	return 0;

err_free_coordinators:
	free_arraylist((*coord)->coordinators_);
err_free_workers:
	free_arraylist((*coord)->workers_);
err_free_coord:
	free(*coord);
	return ret;
}


static void free_coordinator(struct coordinator *coord)
{
	free_arraylist(coord->coordinators_);
	free_arraylist(coord->workers_);
	MPI_Comm_free(&coord->coord_comm_);
	free(coord);
}

int run_coordinator(int array_size, int has_error)
{
	struct coordinator *coord;

	int ret = create_coordinator(&coord, has_error, array_size);

	ret = handle_cside_topology(coord);

	if (ret < 0) {
		free_coordinator(coord);
		return ret;
	}

	ret = handle_cside_task(coord);

	if (ret < 0) {
		free_coordinator(coord);
		return ret;
	}

	free_coordinator(coord);

	return 0;
}