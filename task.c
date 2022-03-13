#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "task.h"
#include "error.h"
#include "util.h"

static int compute_worker_count(struct coordinator *coord, int rank)
{
        int result = 0;

        for (int i = 0; i < coord->world_node_count_; i++) {
                result += coord->topology_[rank][i];
        }

        return result;
}

static int compute_work_matrix(struct coordinator *coord, int ***work_matrix)
{
        // increase the value of array size by 2 since we'll
        // be adding the start and end positions of each
        // segment for each coordinator
        coord->array_size_ += 2;

        // create the array to be processed
        int *v = calloc(coord->array_size_, sizeof(int));

        if (v == NULL)
                return -ELIB;

        for (int i = 0; i < coord->array_size_; i++) {
                v[i] = i;
        }

        // create the work matrix containing
        // vector of slices from target array
        // to be processed by the inidivual coordinators 
        int ret = create_matrix(work_matrix,
                                coord->coord_node_count_, 
                                coord->array_size_);
                
        if (ret < 0)
                return -ELIB;

        int start = 0, end = 0;

        // compute number of workers found in topology
        int worker_count = coord->world_node_count_
                                - coord->coord_node_count_;

        // compute size that will be distributed to each worker
        double chunk_size =
                (double) (coord->array_size_ - 2) / worker_count;

        for (int i = 0; i < coord->coord_node_count_; i++) {
                // compute number of workers attributed to
                // current coordinator
                worker_count = compute_worker_count(coord, i);

                if (i == coord->coord_node_count_ - 1) {
                        end = coord->array_size_ - 2;
                } else {
                        end = start + worker_count * chunk_size;
                }

                (*work_matrix)[i][0] = start + 2;
                (*work_matrix)[i][1] = end + 2;

                memcpy((*work_matrix)[i] + start + 2,
                        v + start,
                        (end - start) * sizeof(int));

                start += worker_count * chunk_size;
        }

        free(v);

        return 0;
}


static int scatter_work(struct coordinator *coord)
{
        int **work_matrix = NULL;

        if (coord->rank_ == coord->leader_) {
                int ret = compute_work_matrix(coord, &work_matrix);

                if (ret < 0)
                        return ret;

        } else {
                // receive array size from parent
                MPI_Recv(&coord->array_size_, 1, MPI_INT,
                        coord->parents_[coord->rank_],
                        0, coord->coord_comm_, NULL);

                int ret = create_matrix(&work_matrix,
                                        coord->coord_node_count_, 
                                        coord->array_size_);
                
                if (ret < 0)
                        return -ELIB;
                
                // receive work matrix from parent
                for (int i = 0; i < coord->coord_node_count_; i++) {
                        MPI_Recv(work_matrix[i], coord->array_size_, MPI_INT,
                                coord->parents_[coord->rank_],
                                0, coord->coord_comm_, NULL);
                }
        }

        // send information to children
        for (int i = 0; i < coord->coord_node_count_; i++) {
                if (coord->rank_ == coord->parents_[i]) {
                        // log message
                        printf("M(%d,%d)\n", coord->rank_, i);
                        fflush(stdout);

                        // send array size
                        MPI_Send(&coord->array_size_, 1, MPI_INT,
                                i, 0, coord->coord_comm_);

                        // send work matrix
                        for (int j = 0; j < coord->coord_node_count_; j++) {
                                // log message
                                printf("M(%d,%d)\n", coord->rank_, i);
                                fflush(stdout);

                                MPI_Send(work_matrix[j], coord->array_size_,
                                        MPI_INT, i, 0, coord->coord_comm_);
                        }
                }
        }

        coord->work_matrix_ = work_matrix;

        return 0;
}

static int process_work(struct coordinator *coord)
{
        int worker_count = get_size(coord->workers_);
        int start, end;
        int *recv_dummy = calloc(coord->array_size_, sizeof(int));

        if (recv_dummy == NULL)
                return -ELIB;
        
        for (int i = 0; i < coord->coord_node_count_; i++) {
                // do not make current node's vector
                // 0 since it will be used in the processing
                if (i == coord->rank_)
                        continue;

                for (int j = 0; j < coord->array_size_; j++) {
                        coord->work_matrix_[i][j] = 0;
                }
        }

        // send array to be processed by each worker
        for (int i = 0; i < worker_count; i++) {
                // extract current worker's rank
                int crt_rank = *(int*)get(coord->workers_, i);

                // log message
                printf("M(%d,%d)\n", coord->rank_, crt_rank);
                fflush(stdout);

                // send size of array to be processed
                MPI_Send(&coord->array_size_, 1, MPI_INT, crt_rank,
                        0, MPI_COMM_WORLD);

                int slice_start = coord->work_matrix_[coord->rank_][0];
                int slice_end = coord->work_matrix_[coord->rank_][1];

                // compute size of the slice to be processed
                int slice_size = slice_end - slice_start;
                
                start = i * (double) slice_size / worker_count + slice_start;
                end = slice_start +
                        fmin((i + 1) * (double) slice_size / worker_count,
                                slice_end);
                
                // log message
                printf("M(%d,%d)\n", coord->rank_, crt_rank);
                fflush(stdout);

                // send start and end positions
                MPI_Send(&start, 1, MPI_INT, crt_rank,
                        0, MPI_COMM_WORLD);
                
                // log message
                printf("M(%d,%d)\n", coord->rank_, crt_rank);
                fflush(stdout);
                
                MPI_Send(&end, 1, MPI_INT, crt_rank,
                        0, MPI_COMM_WORLD);

                // log message
                printf("M(%d,%d)\n", coord->rank_, crt_rank);
                fflush(stdout);
                
                // send array to be processed
                MPI_Send(coord->work_matrix_[coord->rank_], coord->array_size_,
                                MPI_INT, crt_rank, 0, MPI_COMM_WORLD);
        }

        for (int i = 0; i < worker_count; i++) {
                // extract current worker's rank
                int crt_rank = *(int*)get(coord->workers_, i);

                int slice_start = coord->work_matrix_[coord->rank_][0];
                int slice_end = coord->work_matrix_[coord->rank_][1];

                // compute size of the slice to be processed
                int slice_size = slice_end - slice_start;
                
                start = i * (double) slice_size / worker_count + slice_start;
                end = slice_start +
                        fmin((i + 1) * (double) slice_size / worker_count,
                                slice_end);

                // receive processed array
                MPI_Recv(recv_dummy, coord->array_size_, MPI_INT,
                        crt_rank, 0, MPI_COMM_WORLD, NULL);

                // update array
                for (int j = start; j < end; j++) {
                        coord->work_matrix_[coord->rank_][j] = recv_dummy[j];
                }
        }

        free(recv_dummy);
        return 0;
}

static int gather_work(struct coordinator *coord)
{
        int coord_count = coord->coord_node_count_;
        int array_size = coord->array_size_;

        int **recv_dummy = NULL;

        int ret = create_matrix(&recv_dummy,
                                coord_count,
                                array_size);

        if (ret < 0)
                return -ELIB;

        // receive matrices from children and add them
        // to node's matrix
        for (int i = 0; i < coord_count; i++) {
                if (coord->rank_ == coord->parents_[i]) {
                        for (int j = 0; j < coord_count; j++) {
                                MPI_Recv(recv_dummy[j], array_size, MPI_INT,
                                                i, 0, coord->coord_comm_,
                                                NULL);
                        }

                        for (int j = 0; j < coord_count; j++) {
                                for (int k = 0; k < array_size; k++) {
                                        coord->work_matrix_[j][k]
                                                        += recv_dummy[j][k];
                                }
                        }
                }
        }

        // send matrix to parent
        if (coord->rank_ != coord->leader_) {
                for (int i = 0; i < coord_count; i++) {
                        // log message
                        printf("M(%d,%d)\n", coord->rank_,
                                coord->parents_[coord->rank_]);
                        fflush(stdout);

                        MPI_Send(coord->work_matrix_[i], array_size, MPI_INT,
                                        coord->parents_[coord->rank_], 0,
                                        coord->coord_comm_);
                }
        }
        return 0;
}

int cmpfunc (const void *a, const void *b) {
   return (*(int*)a - *(int*)b);
}

static int print_work_result(struct coordinator *coord)
{
        int *result = calloc(coord->array_size_ - 2, sizeof(int));

        if (result == NULL)
                return -ELIB;

        // the result array is computed by summing all the rows in the work_matrix
        // (excluding the first 2 elements in each row because those are not relevant)
        for (int i = 2; i < coord->array_size_; i++) {
                for (int j = 0; j < coord->coord_node_count_; j++) {
                        result[i - 2] += coord->work_matrix_[j][i];
                }
        }

        printf("Rezultat: ");

        // print the result array
        for (int i = 0; i < coord->array_size_ - 2; i++) {
                printf("%d ", result[i]);
        }

        printf("\n");

        fflush(stdout);

        free(result);
        return 0;
}


int handle_cside_task(struct coordinator *coord)
{
        // distribute work to all coordinators
        int ret = scatter_work(coord);

        if (ret < 0)
                return ret;
        
        // wait for scattering to finish
        MPI_Barrier(coord->coord_comm_);

        // process given workload
        ret = process_work(coord);

        if (ret < 0)
                return ret;

        MPI_Barrier(coord->coord_comm_);

        // gather the work result to leader (rank 0)
        ret = gather_work(coord);

        if (ret < 0)
                return ret;

        MPI_Barrier(coord->coord_comm_);

        if (coord->rank_ == 0) {
                int ret = print_work_result(coord);

                if (ret < 0)
                        return ret;
        }

        // free resources used in the task processing
        free_matrix(coord->topology_, coord->coord_node_count_);
        free_matrix(coord->work_matrix_, coord->coord_node_count_);

        return 0;
}


int handle_wside_task(struct worker *worker)
{
        int array_size, start, end, *v;

        // receive size of the array to be processed
        MPI_Recv(&array_size, 1, MPI_INT, worker->coordinator_,
                        0, MPI_COMM_WORLD, NULL);

        // receive start and end indexes
        MPI_Recv(&start, 1, MPI_INT, worker->coordinator_,
                        0, MPI_COMM_WORLD, NULL);
        
        MPI_Recv(&end, 1, MPI_INT, worker->coordinator_,
                        0, MPI_COMM_WORLD, NULL);

        v = calloc(array_size, sizeof(int));

        if (v == NULL)
                return -ELIB;

        // receive the array
        MPI_Recv(v, array_size, MPI_INT, worker->coordinator_,
                        0, MPI_COMM_WORLD, NULL);

        // double the array elements
        for (int i = start; i < end; i++) {
                v[i] *= 2;
        }

        // log message
        printf("M(%d,%d)\n", worker->rank_,
                worker->coordinator_);
        fflush(stdout);

        // send back the processed array
        MPI_Send(v, array_size, MPI_INT, worker->coordinator_,
                        0, MPI_COMM_WORLD);

        free(v);

        return 0;
}