#include <stdlib.h>
#include <string.h>

#include "arraylist.h"
#include "error.h"


#define DEFAULT_CAPACITY 1

struct arraylist {
	void *info_; // information held by the arraylist
	size_t chunk_size_; // size of an element (chunk) in arraylist
	size_t capacity_; // capacity of the arraylist
	size_t csize_; // current size of the arraylist
};


int create_arraylist(arraylist_t *list, size_t chunk_size)
{
	*list = calloc(1, sizeof(struct arraylist));


	if (*list == NULL)
		return -ELIB;

	(*list)->chunk_size_ = chunk_size;
	(*list)->capacity_ = DEFAULT_CAPACITY;

	(*list)->info_ = calloc(DEFAULT_CAPACITY, chunk_size);

	if ((*list)->info_ == NULL) {
		free(*list);
		return -ELIB;
	}

	return 0;
}

int push_back(arraylist_t list, void *info)
{
	if (list->capacity_ == list->csize_) {
		// double the arraylist's capacity
		void *new_info = calloc(2 * list->capacity_,
					list->chunk_size_);

		if (new_info == NULL)
			return -ELIB;

		// copy all elements from old info vector
		memcpy(new_info, list->info_,
		       list->capacity_ * list->chunk_size_);

		free(list->info_);
		list->info_ = new_info;
		list->capacity_ *= 2;
	}
	memcpy(list->info_ + list->chunk_size_ * list->csize_, info,
	       list->chunk_size_);

	list->csize_++;

	return 0;
}

void *get(arraylist_t list, int index)
{
	if (index >= list->capacity_) {
		return NULL;
	} else {
		return list->info_ + list->chunk_size_ * index;
	}
}

void free_arraylist(arraylist_t list)
{
	free(list->info_);
	free(list);
}

size_t get_size(arraylist_t list)
{
	return list->csize_;
}
