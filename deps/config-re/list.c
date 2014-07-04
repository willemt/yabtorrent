/*  Copyright (c) 2006-2008, Philip Busch <vzxwco@gmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "list.h"

node_l *list_get_first_node(node_l **list)
{
	assert(list != NULL);

	return(*list);
}

node_l *list_get_last_node(node_l **list)
{
	assert(list != NULL);

	if(*list == NULL) {
		return(NULL);
	} else {
		return((*list)->prev);
	}
}

void *list_get_first(node_l **list)
{
	assert(list != NULL);

	if(*list == NULL) {
		return(NULL);
	} else {
		return((*list)->data);
	}
}


void *list_get_last(node_l **list)
{
	assert(list != NULL);

	if(*list == NULL) {
		return(NULL);
	} else {
		return((*list)->prev->data);
	}
}

void *list_get_data(node_l **list)
{
	assert(list != NULL);

	return (*list)->data;
}

node_l *list_search(node_l **list, void *data)
{
	node_l *np;

	assert(list != NULL);

	np = list_get_first_node(list);

	while (np != NULL) {
		if (np->data == data)
			return np;

		np = list_get_next_node(list, np);
	}

	return NULL;
}

node_l *list_alloc_node(void *data)
{
	node_l *n = NULL;

	if((n = malloc(sizeof(node_l))) != NULL) {
		n->data = data;
	}

	return(n);
}


void list_free_node(node_l *node)
{
	free(node);
}


static void list_link_before(node_l **list,
                             node_l  *here,
                             node_l  *node)
{
	assert(list != NULL);
	assert(node != NULL);

	if(*list == NULL) {
		node->prev = node;
		node->next = node;
		*list = node;
	} else {
		assert(here != NULL);

		node->prev = here->prev;
		node->next = here;
		here->prev = node;
		node->prev->next = node;

		if(here == *list) {
			*list = node;
		}
	}
}

/*
static void list_link_after(node_l **list,
                            node_l  *here,
                            node_l  *node)
{
	assert(list != NULL);
	assert(node != NULL);

	if(*list == NULL) {
		node->prev = node;
		node->next = node;
		*list = node;
	} else {
		assert(here != NULL);

		node->prev = here;
		node->next = here->next;
		here->next = node;
		node->next->prev = node;
	}
}
*/

int list_prepend(node_l **list,
                 void    *data)
{
	node_l *n = NULL;

	if((n = list_alloc_node(data)) == NULL) {
		return(-1);
	} else {
		list_link_before(list, *list, n);
		return(0);
	}
}


int list_append(node_l **list,
                void    *data)
{
	if(list_prepend(list, data) < 0) {
		return(-1);
	} else {
		*list = (*list)->next;
		return(0);
	}
}


void list_prepend_node(node_l **list,
                       node_l  *node)
{
	assert(list != NULL);
	assert(node != NULL);

	list_link_before(list, *list, node);
}


void list_append_node(node_l **list,
                     node_l  *node)
{
	assert(list != NULL);
	assert(node != NULL);

	list_prepend_node(list, node);
	*list = (*list)->next;
}

void list_join(node_l **list_a,
               node_l **list_b)
{
	node_l *first_a, *first_b, *last_a, *last_b;

	assert(list_a != NULL);
	assert(list_b != NULL);

	if(list_is_empty(list_b))
		return;

	first_a = list_get_first_node(list_a);
	first_b = list_get_first_node(list_b);
	last_a = list_get_last_node(list_a);
	last_b = list_get_last_node(list_b);

	first_a->prev = last_b;
	last_a->next = first_b;

	first_b->prev = last_a;
	last_b->next = first_a;
}


int list_size(node_l **list)
{
	int s = 0;
	node_l *n;

	assert(list != NULL);

	n = *list;

	while(n != NULL) {
		n = list_get_next_node(list, n);
		s++;
	}

	return(s);	
}


int list_is_empty(node_l **list)
{
	assert(list != NULL);

	return(*list == NULL);
}


void list_foreach(node_l **list,
                 void     func(void *))
{
	node_l *n;

	assert(list != NULL);

	n = list_get_first_node(list);

	while(n != NULL) {
		func(n->data);
		n = list_get_next_node(list, n);
	}
}


void list_unlink(node_l **list,
                 node_l  *node)
{
	assert(list != NULL);
	assert(node != NULL);

	if(node->next == node) {
		*list = NULL;
	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;

		if(*list == node)
			*list = node->next;
	}

	node->next = NULL;
	node->prev = NULL;
}


void list_split(node_l **list,
               node_l **front,
               node_l **back)
{
	node_l *fast, *slow, *first, *last;

	assert(list  != NULL);
	assert(front != NULL);
	assert(back  != NULL);

	if((*list == NULL) || ((*list)->next == *list)) {
		*back = NULL;
	} else {
		slow = list_get_first_node(list);
		fast = list_get_next_node(list, slow);
		
		while(fast != NULL) {
			fast = list_get_next_node(list, fast);
			if(fast != NULL) {
				slow = list_get_next_node(list, slow);
				fast = list_get_next_node(list, fast);
			}
		}

		first = list_get_first_node(list);
		last  = list_get_last_node(list);

		*back = list_get_next_node(list, slow);

		(*back)->prev = last;
		last->next = *back;

		slow->next = first;
		first->prev = slow;
	}

	*front = *list;
	*list = NULL;
}


node_l *list_pop_first_node(node_l **list)
{
	node_l *n;

	assert(list != NULL);

	n = list_get_first_node(list);

	if(n != NULL)
		list_unlink(list, n);

	return(n);
}


node_l *list_pop_last_node(node_l **list)
{
	node_l *n;

	assert(list != NULL);

	n = list_get_last_node(list);

	if(n != NULL)
		list_unlink(list, n);

	return(n);
}


void list_merge(node_l **dest,
               node_l **list_a,
               node_l **list_b,
               int      cmp(void *, void *))
{


	assert(dest   != NULL);
	assert(list_a != NULL);
	assert(list_b != NULL);

	for(;;) {
		if(list_is_empty(list_a)) {
			list_join(dest, list_b);
			break;
		}

		if(list_is_empty(list_b)) {
			list_join(dest, list_a);
		        break;
		}
		
		if(cmp(list_get_first(list_a), list_get_first(list_b)) <= 0) {
			list_append_node(dest, list_pop_first_node(list_a));
		} else {
			list_append_node(dest, list_pop_first_node(list_b));
		}
	}
}


void list_sort(node_l **list,
               int cmp(void *, void *))
{
	node_l *left, *right;

	assert(list != NULL);

	if(list_size(list) < 2)
		return;

	list_split(list, &left, &right);

	list_sort(&left, cmp);
	list_sort(&right, cmp);

	list_merge(list, &left, &right, cmp);
}


int list_copy(node_l **src,
              node_l **dest)
{
	node_l *n;

	assert(dest != NULL);
	assert(src  != NULL);
	assert(src  != dest);

	*dest = NULL;

	n = *src;

	while(n != NULL) {
		if(list_append(dest, n->data)) {
			/* FIXME: delete list */
			return(-1);
		}

		n = list_get_next_node(src, n);
	}

	return(0);
}

void list_destroy(node_l **list)
{
	assert(list != NULL);

	size_t size, i;

	node_l *n;

	size = list_size(list);
	n = list_get_first_node(list);

	for (i = 0; i < size - 1; i++) {
		n = list_get_next_node(list, n);
		free(n->prev);
	}

	free(n);
}


/*
int node_cmp(void *a, void *b)
{
	testdata *lx = (testdata *)a, *ly = (testdata *)b;

	if(lx->n < ly->n) return(-1);
	if(lx->n == ly->n) return(0);
	if(lx->n > ly->n) return(1);

	return(0);
}

static node_l *list_create_123()
{
	node_l *n = NULL;
	testdata *a=malloc(sizeof(testdata)),
	     *b=malloc(sizeof(testdata)),
	     *c=malloc(sizeof(testdata));

	a->n = 1;
	strcpy(a->str, "foo");
	b->n = 2;
	strcpy(b->str, "bar");
	c->n = 3;
	strcpy(c->str, "snafu");

	list_append(&n, a);
	list_append(&n, b);
	list_append(&n, c);

	return(n);
}


static void print_data(void *foo)
{
	testdata *d = (testdata *)foo;

	printf("%d - %s\n", d->n, d->str);
}


int main()
{
	testdata d;
	node_l *list = list_create_123();
	node_l *n = list_alloc_node(&d);
	node_l *list2 = list_create_123();
	node_l *copy;

	d.n = 4;
	strcpy(d.str, "tralala");

	

	list_append_node(&list, n);

	list_join(&list, &list2);
	list_sort(&list, node_cmp);

	printf("%d\n", list_size(&list));
	list_foreach(&list, print_data);

	list_copy(&list, &copy);
	printf("%d\n", list_size(&copy));
	list_foreach(&copy, print_data);

	return(0);
}
*/
