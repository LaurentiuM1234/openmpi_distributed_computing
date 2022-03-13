#include <stdlib.h>

#include "util.h"
#include "error.h"


void free_matrix(int **matrix, int size)
{
	for (int i = 0; i < size; i++) {
		free(matrix[i]);
	}

	free(matrix);
}

int create_matrix(int ***matrix, int rows, int columns)
{
	*matrix = calloc(rows, sizeof(int*));

	if (*matrix == NULL)
		return -ELIB;

	for (int i = 0; i < rows; i++) {
		(*matrix)[i] = calloc(columns, sizeof(int));

		if ((*matrix)[i] == NULL) {
			free_matrix(*matrix, i);
			return -ELIB;
		}
	}

	return 0;
}