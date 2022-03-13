#ifndef _TOPO_H_
#define _TOPO_H_

#include "coordinator.h"
#include "worker.h"

/**
 * @brief Handle all topology-related operations on coordinator
 *        node's side
 * 
 * @param coord structure containing information about current
 *              coordinator node
 * @return -ELIB - if any of the system calls fails
 *         0 - upon success
 */
int handle_cside_topology(struct coordinator *coord);

/**
 * @brief Handle all topology-related operations on worker
 *        node's side
 * 
 * @param coord structure containing information about current
 *              worker node
 * @return -ELIB - if any of the system calls fails
 *         0 - upon success
 */
int handle_wside_topology(struct worker *worker);

#endif // _TOPO_H_