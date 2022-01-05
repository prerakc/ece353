#include "request.h"
#include "server_thread.h"
#include "common.h"

pthread_cond_t full = PTHREAD_COND_INITIALIZER; 
pthread_cond_t empty = PTHREAD_COND_INITIALIZER; 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache = PTHREAD_MUTEX_INITIALIZER;

struct entry {
	struct file_data *data;
	struct entry *next;
}; 

struct LRU {
	char* name;
	int size;
	struct LRU *next;
};

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	/* add any other parameters you need */
	int *req_buf;
	int buf_count;
	int buf_in;
	int buf_out;
	pthread_t *w_threads;
	int cache_size;
	struct entry **cache;
	struct LRU *LRU;
};

/* static functions */

static unsigned long get_hash(char *str, int num_buckets) {
	unsigned long hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash % num_buckets;
}

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static struct entry* cache_lookup(struct server *sv, char *file) {
	unsigned long key = get_hash(file, sv->max_cache_size);

	struct entry *node = sv->cache[key];

	while (node != NULL) {
		if (strcmp(node->data->file_name, file) == 0) {
			break;
		}

		node = node->next;
	}

	// update lru as long as there are > 1 items
	if (node != NULL && sv->LRU->next != NULL) {
		struct LRU *prev = NULL;
		struct LRU *curr = sv->LRU;

		while (curr != NULL) {
			if (strcmp(file, curr->name) == 0) {
				if (prev == NULL) {
					sv->LRU = sv->LRU->next;
				} else {
					prev->next = curr->next;
				}

				curr->next = NULL;
			}

			prev = curr;
			curr = curr->next;
		}
		
		struct LRU *end = sv->LRU;

		while (end->next != NULL) {
			end = end->next;
		}

		end->next = prev;
	}

	return node;
}

static void cache_evict(struct server *sv, int amount_to_evict) {
	int amount_evicted = 0;

	while (amount_evicted < amount_to_evict) {
		struct LRU *node = sv->LRU;
		
		amount_evicted = amount_evicted + node->size;

		unsigned long key = get_hash(node->name, sv->max_cache_size);

		struct entry *prev = NULL;
		struct entry *curr = sv->cache[key];

		while (curr != NULL) {
			if (strcmp(curr->data->file_name, node->name) == 0) {
				if (prev == NULL) {
					sv->cache[key] = sv->cache[key]->next;
				} else {
					prev->next = curr->next;
				}

				curr->next = NULL;
			}

			prev = curr;
			curr = curr->next;
		}

		file_data_free(prev->data);
		free(prev);

		sv->LRU = sv->LRU->next;

		free(node->name);
		free(node);
	}
	
	sv->cache_size = sv->cache_size - amount_evicted;
}

static struct entry* cache_insert(struct server *sv, struct file_data *file) {	
	if (file->file_size > sv->max_cache_size/10) {
		return NULL;
	}

	int amount_to_evict = sv->cache_size + file->file_size - sv->max_cache_size;

	if (amount_to_evict > 0) {
		cache_evict(sv, amount_to_evict);
	}
	
	sv->cache_size = sv->cache_size + file->file_size;
	unsigned long key = get_hash(file->file_name, sv->max_cache_size);

	struct entry* new_entry = malloc(sizeof(struct entry));
	new_entry->data = file_data_init();
	new_entry->data->file_buf = strdup(file->file_buf);
	new_entry->data->file_name = strdup(file->file_name);
	new_entry->data->file_size = file->file_size;
    new_entry->next = NULL;

	if(sv->cache[key] == NULL) {
		sv->cache[key] = new_entry;
	} else {
		struct entry *prev = sv->cache[key];
		struct entry *curr = prev->next;

		while (curr != NULL) {
			prev = curr;
			curr = curr->next;
		}

		prev->next = new_entry;
	}

	// add to lru
	struct LRU *new_LRU = malloc(sizeof(struct LRU));
	new_LRU->name = strdup(file->file_name);
	new_LRU->size = file->file_size;
	new_LRU->next = NULL;
	
	if (sv->LRU == NULL) {
		sv->LRU = new_LRU;
	} else {
		struct LRU *prev = sv->LRU;
		struct LRU *curr = prev->next;

		while (curr != NULL) {
			prev = curr;
			curr = curr->next;
		}

		prev->next = new_LRU;
	}

	return new_entry;
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}

	if (sv->max_cache_size == 0) {
		/* read file, 
		* fills data->file_buf with the file contents,
		* data->file_size with file size. */
		ret = request_readfile(rq);

		if (ret != 0) { /* successfully read file */
			/* send file to client */
			request_sendfile(rq);
		}

		goto out;
	}

	pthread_mutex_lock(&cache);

	struct entry *cache_ret = cache_lookup(sv, data->file_name);

	if (cache_ret != NULL) {
		request_set_data(rq,cache_ret->data);
	} else {
		pthread_mutex_unlock(&cache);
		
		ret = request_readfile(rq);

		if (ret == 0) {
			goto out;
		}

		pthread_mutex_lock(&cache);

		cache_ret = cache_lookup(sv, data->file_name);

		if (cache_ret == NULL) {
			cache_ret = cache_insert(sv, data);
		}
	}
	
	pthread_mutex_unlock(&cache);

	request_sendfile(rq);

out:
	request_destroy(rq);
	file_data_free(data);
}

/* entry point functions */

void *thread_main(void *args) {
	struct server *sv = (struct server *)args;
	
	for (;;) {
		pthread_mutex_lock(&lock);
		
		while (sv->buf_count == 0) {
			pthread_cond_wait(&empty, &lock);
			
			if (sv->exiting == 1) {
				pthread_mutex_unlock(&lock);
				pthread_exit(NULL);
			}
		}
		
		int connfd = sv->req_buf[sv->buf_out];
		
		if (sv->buf_count == sv->max_requests) {
			pthread_cond_broadcast(&full);
		}
		
		sv->buf_out = (sv->buf_out + 1) % sv->max_requests;
		sv->buf_count = sv->buf_count - 1;
		
		pthread_mutex_unlock(&lock);
		
		do_server_request(sv, connfd);
	}
}

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	pthread_mutex_lock(&lock);
	
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
	
	sv->req_buf = NULL;
	sv->buf_count = 0;
	sv->buf_in = 0;
	sv->buf_out = 0;
	sv->w_threads = NULL;
	sv->cache_size = 0;
	sv->cache = NULL;
	sv->LRU = NULL;

	/* Lab 4: create queue of max_request size when max_requests > 0 */
	if (max_requests > 0) {
		sv->req_buf = malloc(sizeof(int) * max_requests);
	}

	/* Lab 5: init server cache and limit its size to max_cache_size */
	if (max_cache_size > 0) {
		sv->cache = malloc(sizeof(struct entry *) * max_cache_size);

		for (int i = 0; i < max_cache_size; i++) {
			sv->cache[i] = NULL;
		}
	}

	/* Lab 4: create worker threads when nr_threads > 0 */
	if (nr_threads > 0) {
		sv->w_threads = malloc(sizeof(pthread_t) * nr_threads);	

		for(int i = 0; i < nr_threads; i++) {
			pthread_create(&(sv->w_threads[i]), NULL, (void *) thread_main, (void *) sv);
		}
	}
	
	pthread_mutex_unlock(&lock);
	
	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(&lock);
		
		while (sv->buf_count == sv->max_requests) {
			pthread_cond_wait(&full, &lock);
			
			if (sv->exiting == 1) {
				pthread_mutex_unlock(&lock);
				pthread_exit(NULL);
			}
		}
		
		sv->req_buf[sv->buf_in] = connfd;
				
		if (sv->buf_count == 0) {
			pthread_cond_broadcast(&empty);
		}
		
		sv->buf_in = (sv->buf_in + 1) % sv->max_requests;
		sv->buf_count = sv->buf_count + 1;
				
		pthread_mutex_unlock(&lock);
	}
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	sv->exiting = 1;

	pthread_cond_broadcast(&empty);
	pthread_cond_broadcast(&full);
	
	for(int i = 0; i < sv->nr_threads; i++) {
		pthread_join(sv->w_threads[i], NULL);
	}

	/* make sure to free any allocated resources */
	free(sv->w_threads);
	free(sv->req_buf);

	for (int i = 0; i < sv->max_cache_size; i++) {
		struct entry *ptr = sv->cache[i];
		struct entry *tmp;

		while (ptr != NULL) {
			tmp = ptr;
			ptr = ptr->next;
			file_data_free(tmp->data);
			free(tmp);
		}
	}
	free(sv->cache);

	struct LRU *ptr = sv->LRU;
	struct LRU *tmp;

	while (ptr != NULL) {
		tmp = ptr;
		ptr = ptr->next;
		free(tmp->name);
		free(tmp);
	}
	free(sv->LRU);
	
	free(sv);
}