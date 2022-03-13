#include <stdlib.h>
#include <stdio.h>

#include "worker.h"
#include "error.h"
#include "util.h"
#include "topo.h"
#include "task.h"


#define WORKER_COLOR 1
#define COORD_COUNT 3


static int add_node_info(struct worker *worker)
{
        // set number of coordinator nodes
        worker->coord_node_count_ = COORD_COUNT;

        // extract rank of worker using world communicator
	MPI_Comm_rank(MPI_COMM_WORLD, &worker->rank_);

        // extract number of nodes in world communicator
	MPI_Comm_size(MPI_COMM_WORLD, &worker->world_node_count_);

        MPI_Comm worker_comm;
        int worker_count = worker->world_node_count_
                                - worker->coord_node_count_;

        // create new communicator used only by the worker
        // (this communicator is created because the coordinator nodes
        // are grouped in the same communicator and the framework
        // expects that all nodes call @MPI_Comm_split otherwise the
        // other nodes will be blocked)
	MPI_Comm_split(MPI_COMM_WORLD,
			WORKER_COLOR,
			worker->rank_% worker_count,
			&worker_comm);

        MPI_Comm_free(&worker_comm);

        return 0;
}

static int create_worker(struct worker **worker)
{
        *worker = calloc(1, sizeof(struct worker));

        if (*worker == NULL)
                return -ELIB;

        int ret = add_node_info(*worker);

        if (ret < 0)
                goto err_free_worker;

        return 0;

err_free_worker:
        free(*worker);
        return ret;
}

static void free_worker(struct worker *worker)
{
        free(worker);
}

int run_worker()
{
        struct worker *worker;

        int ret = create_worker(&worker);

        if (ret < 0)
                return ret;

        ret = handle_wside_topology(worker);

        if (ret < 0) {
                free_worker(worker);
                return ret;
        }

        ret = handle_wside_task(worker);

        if (ret < 0) {
                free_worker(worker);
                return ret;
        }

        free_worker(worker);

        return 0;
}