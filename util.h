#ifndef _UTIL_H_
#define _UTIL_H_

/**
 * @brief Allocate memory for a matrix of given size
 * 
 * @param matrix address of matrix to be alloc'd
 * @param rows number of rows in matrix
 * @param columns number of columns in matrix
 * @return -ELIB - if any of the system calls fails
 *         0 - upon success
 */
int create_matrix(int ***matrix, int rows, int columns);

/**
 * @brief Free memory occupied by matrix
 * 
 * @param matrix matrix to be free'd
 * @param size number of rows in the matrix
 */
void free_matrix(int **matrix, int size);

#endif // _UTIL_H_