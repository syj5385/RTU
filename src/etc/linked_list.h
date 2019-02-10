/*
 * ADT_list.h
 *
 *  Created on: 2016. 10. 28.
 *      Author: jjunj
 */

#ifndef _ETC_LINKED_LIST_
#define _ETC_LINKED_LIST_

#include <stdio.h>
#include <stdlib.h>


// LIST_node
typedef struct node{
	void* data;
	struct node* next;
}NODE;


// LINKED LIST
typedef struct list{
	int count;
	NODE* front;
	NODE* rear;
	NODE* pos;
}LLIST;


// INTERFACE
LLIST* create_list();
int add_node_at(LLIST* list, unsigned int index, void* in);
int del_node_at(LLIST* list, unsigned int index);
void* get_data_at(LLIST* list, unsigned int index);
void delete_all_list(LLIST* list);

#endif /* _ETC_LINKED_LIST_ */
