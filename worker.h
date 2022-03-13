#ifndef _WORKER_H_
#define _WORKER_H_

#include <mpi.h>

struct worker {
        int coordinator_; // rank of the connected coordinator node

        int rank_; // rank of the worker
        int world_node_count_; // number of nodes in the world communicator
        int coord_node_count_; // number of coordinator nodes
};


/**
 * @brief Run the worker node tasks
 * 
 * @return -ELIB - if any of the system calls failed
 * 	   0 - upon success
 */
int run_worker();


#endif // _WORKER_H_