
#include <stdlib.h>
#include <string.h>
#include "list.h"

static list_item _newitem(list lp, void *item,long size) {
	list_item new_item;

	new_item = (list_item) malloc(LIST_ITEM_SIZE);
	if (!new_item) return 0;

	new_item->item = (char *) malloc((size_t)size);
	if (!new_item->item) {
		free(new_item);
		return 0;
	}

	memcpy((void *)new_item->item,(void *)item,(size_t)size);
	new_item->size = size;
	new_item->prev = (list_item) 0;
	new_item->next = (list_item) 0;

	return new_item;
}

void *list_add(list lp,void *item,long size) {
	list_item ip, new_item;

	if (!lp) return 0;

	/* Create a new item */
	new_item = _newitem(lp,item,size);
	if (!new_item) return 0;


	/* Add it to the list */
	if (!lp->first)
		lp->first = lp->last = lp->next = new_item;
	else {
		ip = lp->last;                  /* Get last item */
		ip->next = new_item;            /* Add this one */
		new_item->prev = ip;		/* Point to it */
		lp->last = new_item;            /* Make this last */
	}
	return new_item->item;
}

int list_add_list(list lp,list lp2) {
	list_item ip;

	if (!lp) return 1;

	/* Get each item from the old list and copy it to the new one */
	ip = lp2->first;
	while(ip) {
		list_add(lp,ip->item,ip->size);
		ip = ip->next;
	}

	/* Return the new list */
	return 0;
}

int list_count(list lp) {
	list_item ip;
	register int count;

	if (!lp) return -1;

	count = 0;
	for(ip = lp->first; ip; ip = ip->next) count++;
	return count;
}

list list_create(void) {
	list lp;

	lp = (list) malloc(LIST_SIZE);
	if (!lp) return 0;

	lp->first = lp->last = lp->next = (list_item) 0;
	return lp;
}

int list_delete(list lp,void *item) {
	list_item ip,prev,next;

	if (!lp) return -1;


	ip = lp->first;
	while(ip) {
		if (item == ip->item) {
			prev = ip->prev;        /* Get the pointers */
			next = ip->next;

			/* Fixup links in other first */
			if (prev) prev->next = next;
			if (next) next->prev = prev;

			/* Was this the 1st item? */
			if (ip == lp->first) lp->first = next;

			/* Was this the last item? */
			if (ip == lp->last) lp->last = prev;

			/* Was this the next item? */
			if (ip == lp->next) lp->next = next;


			free(item);		/* Free the item */
			free(ip);		/* Free the ptr */
			return 0;		/* Item found and deleted */
		}
		ip = ip->next;                  /* Next item, please. */
	}
	return 1;                               /* Item not found, error. */
}



int list_destroy(list lp) {
	list_item ip,next;

	if (!lp) return -1;

	ip = lp->first;                         /* Start at beginning */
	while(ip) {
		free(ip->item);			/* Free the item */
		next = ip->next;                /* Get next pointer */
		free(ip);			/* Free current item */
		ip = next;                      /* Set current item to next */
	}
	free(lp);				/* Free list */
	return 0;
}



list list_dup(list lp) {
	list new_list;
	list_item ip;

	if (!lp) return 0;

	/* Create a new list */
	new_list = list_create();
	if (!new_list) return 0;

	/* Get each item from the old list and copy it to the new one */
	ip = lp->first;
	while(ip) {
		list_add(new_list,ip->item,ip->size);
		ip = ip->next;
	}

	/* Return the new list */
	return new_list;
}



void *list_get_next(list lp) {
	list_item ip;
	void *item;

	if (!lp) return 0;

	item = 0;
	if (lp->next) {
		ip = lp->next;
		lp->next = ip->next;
		item = ip->item;
	}
	return item;
}

int list_is_next(list lp) {
	if (!lp) return -1;
	return(lp->next ? 1 : 0);
}



int list_reset(list lp) {

	if (!lp) return -1;

	lp->next = lp->first;
	return 0;
}



static int _compare(list_item item1, list_item item2) {
	register int val;

	val = strcmp(item1->item,item2->item);
	if (val < 0)
		val =  -1;
	else if (val > 0)
		val = 1;
	return val;
}

/* Sorts the items in a list -- NOTE: all items MUST be strings! */
int list_sort(list lp, list_compare compare, int order) {
	int comp,swap;
	list_item t;
	register list_item ip,ip2;

	if (!lp) return -1;

	/* If compare is null, use internal compare (strcmp) */
	if (!compare) {
		compare = _compare;
	}

	/* Set comp negative or positive, depending upon order */
	comp = (order == 0 ? 1 : -1);

	/* Sort the list */
	for(ip = lp->first; ip;) {
		swap = 0;
		for(ip2 = ip->next; ip2; ip2 = ip2->next) {
			if (compare(ip,ip2) == comp) {
				swap = 1;
				break;
			}
		}
		if (swap) {
			/* 'unhook' ip2 */
			if (ip2->prev) {
				t = ip2->prev;
				t->next = ip2->next;
			}
			if (ip2->next) {
				t = ip2->next;
				t->prev = ip2->prev;
			}
			/* put ip2 before ip */
			if (ip->prev) {
				t = ip->prev;
				t->next = ip2;
			}
			ip2->prev = ip->prev;
			ip2->next = ip;
			ip->prev = ip2;
			ip = ip2;
			if (!ip->prev) lp->first = ip;
			if (!ip->next) lp->last = ip;
		} else
			ip = ip->next;
	}
	return 0;
}
