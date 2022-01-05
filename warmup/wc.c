#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>

struct wc {
	/* you can define this struct to have whatever fields you want. */
	struct entry **table;
	int buckets;
};

struct entry {
	char *word;
	int count;
	struct entry *next;
};

void insert (struct wc *wc, char *word);
unsigned long hash(char *str, int num_buckets);

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
	 
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
	
	int words = 0;
	int prev = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < size; i++) {
		if (isspace(word_array[i])) {
			if (isspace(word_array[prev])) {
				continue;
			}
			
			//for (j = prev; j < i; j++) {
			//	printf("%c", word_array[j]);

			//}
			//printf("\n");

			words += 1;
			prev = i;
		} else {
			if (isspace(word_array[prev])) {
				prev = i;
				continue;
			}

			if (i == size - 1) {
				//for (j = prev; j <= i; j++) {
				//	printf("%c", word_array[j]);
				//}
				//printf("\n");
					
				words += 1;
			}
		}
	}

	words *= 2;
	
	wc->table = malloc(sizeof(struct entry *)*words);
	wc->buckets = words;

	for (i = 0; i < words; i++) {
		wc->table[i] = NULL;
	}

	prev = 0;

	int len = 0;
	char *word = NULL;

	for (i = 0; i < size; i++) {
		if (isspace(word_array[i])) {
			if (isspace(word_array[prev])) {
				continue;
			}
			
			len = i - prev + 1;
			word = malloc(sizeof(char)*len);

			for (j = 0; j < i - prev; j++) {
				word[j] = word_array[j + prev];

			}
			word[i - prev] = '\0';
			
			insert(wc, word);

			prev = i;
		} else {
			if (isspace(word_array[prev])) {
				prev = i;
				continue;
			}

			if (i == size - 1) {
				len = i - prev + 1;
				word = malloc(sizeof(char)*len);

				for (j = 0; j <= i - prev; j++) {
					word[j] = word_array[j + prev];
				}
					
				insert(wc, word);
			}
		}
	}
	

	return wc;
}

void insert (struct wc *wc, char *word) {
	unsigned long key = hash(word, wc->buckets);
	
	//printf("%lu : ", key);
	//puts(word);
	
	if (wc->table[key] == NULL) {
		wc->table[key] = malloc(sizeof(struct entry));
		wc->table[key]->word = word;
		wc->table[key]->count = 1;
		wc->table[key]->next = NULL;
		return ;
	}

	struct entry *head = wc->table[key];
	struct entry *prev;

	while (head != NULL) {
		if (strcmp(head->word, word) == 0) {
			head->count += 1;
			return ;
		}

		prev = head;
		head = head->next;
	}

	prev->next = malloc(sizeof(struct entry));
	prev->next->word = word;
	prev->next->count = 1;
	prev->next->next = NULL;
}

unsigned long hash(char *str, int num_buckets) {
	unsigned long hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash % num_buckets;
}

void
wc_output(struct wc *wc)
{
	for (int i = 0; i < wc->buckets; i++) {
		struct entry *ptr = wc->table[i];

		while (ptr != NULL) {
			printf("%s:%d\n", ptr->word, ptr->count);
			ptr = ptr->next;
		}
	}
}

void
wc_destroy(struct wc *wc)
{
	for (int i = 0; i < wc->buckets; i++) {
		struct entry *ptr = wc->table[i];
		struct entry *tmp;

		while (ptr != NULL) {
			tmp = ptr;
			ptr = ptr->next;
			free(tmp);
		}
	}
	free(wc->table);
	free(wc);
}
