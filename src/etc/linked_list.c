/*
 * ADT_list.c
 *
 *  Created on: 2016. 10. 28.
 *      Author: jjunj
 */

#include "linked_list.h"

#include <stdio.h>

// create_list

LLIST* create_list(){
	LLIST* list = (LLIST*)malloc(sizeof(LLIST));
	if(!list)	// list creation check
		return NULL;

	list -> count = 0;
	list -> front = NULL;
	list -> rear = NULL;
	list -> pos = NULL;

	return list;
}

// add_node_at

int add_node_at(LLIST* list, unsigned int index, void* in){
	if(index > ( list -> count)) //check index validation
		return 0;

	NODE* node_ptr = (NODE*)malloc(sizeof(NODE)); // node creation

	if(!node_ptr)
		return 0; // node creation check
	node_ptr -> data = in;
	node_ptr -> next = NULL;

	if( list -> count == 0 ){
		list -> front = node_ptr;
		list -> rear = node_ptr;
		list -> count ++;

		return 1;
	}

	if( index == 0 ){
		node_ptr -> next = list -> front;
		list -> front = node_ptr;
		list -> count ++;

		return 1;
	}

	else{
		list -> pos = list -> front;
		unsigned int i = 1; // index check

		while( i != index ){
			list -> pos = list -> pos -> next;
			i++;
		}

		node_ptr -> next = list -> pos -> next;
		list -> pos -> next = node_ptr;
		if(index == list -> count)
			list -> rear = node_ptr;

		list -> count ++;

		return 1;
	}
}

//del_node_at
int del_node_at(LLIST* list, unsigned int index){
	if(list -> count == 0)	 //checking empty state
		return 0;

	if(index >= list -> count) // check index validation
		return 0;


	NODE* pre;
	list -> pos = list -> front;

	if( index == 0 ){
		list -> front = list -> front -> next;
		free(list->pos->data);
		free(list -> pos);
		list -> count --;

		return 1;
	}

	int i = 0;
	while( i != index){
		pre = list -> pos;
		list -> pos = list -> pos -> next;
		i++;
	}
	pre -> next = list -> pos -> next;
	free(list->pos->data);
	free(list->pos);
	if(i == (list->count-1))
		list -> rear = pre;

	list -> count --;
	return 1;
}

void delete_all_list(LLIST* list){
	int count ; 
	while(list -> count != 0){
		del_node_at(list,0);
	}

	free(list);

}

// get_data_at
void* get_data_at(LLIST* list, unsigned int index){
	if(index >= list -> count) // check index validation
		return 0;

	list -> pos = list -> front;
	int i = 0;

	while(i != index){
		list -> pos = list -> pos -> next;
		i++;
	}
	if(list -> pos == NULL) return 0;

	return list -> pos -> data;
}
