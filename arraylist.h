#ifndef _ARRAYLIST_H_
#define _ARRAYLIST_H_

#include <stdio.h>

typedef struct arraylist* arraylist_t;

/**
 * @brief Create an arraylist object
 * 
 * @param list address of pointer to be alloc'd
 * @param chunk_size size of element found in arraylist
 * @return -ELIB - if any of the system calls failed
 *          0 - upon success
 */
int create_arraylist(arraylist_t *list, size_t chunk_size);

/**
 * @brief Add an element at the back of the arraylist
 * 
 * @param list arraylist in which the element is to be added
 * @param info element to be added
 * @return -ELIB - if any of the system calls failed
 *          0 - upon success
 */
int push_back(arraylist_t list, void *info);

/**
 * @brief Retrieve element from arraylist found at given index
 * 
 * @param list arraylist from which the element is retrieved
 * @param index index of the elment to retrieve
 * @return Address of searched element or NULL if the index is out
 *         of bounds
 */
void *get(arraylist_t list, int index);

/**
 * @brief Free memory occupied by arraylist object
 * 
 * @param list arraylist to be free'd
 */
void free_arraylist(arraylist_t list);

/**
 * @brief Get the size of the arraylist
 * 
 * @param list arraylist object
 * @return Size of the arraylist
 */
size_t get_size(arraylist_t list);

#endif // _ARRAYLIST_H_
