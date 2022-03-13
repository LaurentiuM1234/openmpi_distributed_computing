#ifndef _TASK_H_
#define _TASK_H_


#include "coordinator.h"
#include "worker.h"

/**
 * @brief Handle all task-related operations on coordinator
 *        node's side
 * 
 * @param coord structure containing information about current
 *              coordinator node
 * @return -ELIB - if any of the system calls fails
 *         0 - upon success
 */
int handle_cside_task(struct coordinator *coord);

/**
 * @brief Handle all task-related operations on worker
 *        node's side
 * 
 * @param worker structure containing information about current
 *              worker node
 * @return -ELIB - if any of the system calls fails
 *         0 - upon success
 */
int handle_wside_task(struct worker *worker);


#endif // _TASK_H_