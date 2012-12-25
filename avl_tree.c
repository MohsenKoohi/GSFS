#include "gsfs.h"

#ifndef __AVL_SETTINGS
	#include <stdlib.h>
	#include <string.h>
	#include <stdio.h>

	#define __AVL_SETTINGS
	
	#define avl_data_type		int
	#define avl_data_compare(x,y)	((y)-(x))
	#define avl_data_free(x)		
	#define avl_search_compare(x,y)	((y)-(x))
	#define	avl_search_input_type	int

	struct avl_tree_node{
		struct avl_tree_node	//*parent,
					*left,
					*right;
		avl_data_type data;
	};

	typedef struct avl_tree_node atn;	
#else
	#define malloc(p)	kzalloc(p,GFP_KERNEL)
	#define free(p)		kfree(p)
	#define printf(...)	printk("<0>" __VA_ARGS__)
#endif

atn* avl_tree_search(atn* node,avl_search_input_type data){
	int i;
	
	if(node){
		i=avl_search_compare(node->data,data);
		if(i==0)
			return node;
		if(i<0)
			return avl_tree_search(node->left,data);
		return avl_tree_search(node->right,data);
	}
	return 0;
}

int get_height(atn* node){
	int 	rh,
		lh;
	if(node){
		rh=get_height(node->right);
		lh=get_height(node->left);
		return 1+max(rh,lh);
	}
	return 0;
}

atn* reconstruct(atn* node,int lh,int rh){
	int 	crh,
		clh;		
	atn	*child,
		*gchild,
		*ret;
		
	if(lh-rh==-2){
		//right-right and right-left
		crh=get_height(node->right->right);
		clh=get_height(node->right->left);

		if(clh-crh==1){
			//right-left
			child=node->right;
			gchild=child->left;
			
			node->right=gchild;
		
			child->left=gchild->right;
			
			gchild->right=child;
		}
		//right right
		child=node->right;
		
		node->right=child->left;
		
		child->left=node;
		
		ret=child;
	}
	else{
		//left-left and left-right
		crh=get_height(node->left->right);
		clh=get_height(node->left->left);
		
		if(clh-crh==-1){
			//left-right
			child=node->left;
			gchild=child->right;
			
			node->left=gchild;
			
			child->right=gchild->left;
			
			gchild->left=child;
		}
		//left-left
		child=node->left;
		
		node->left=child->right;
		
		child->right=node;
		
		ret=child;
	}
	return ret;
}

atn* insert_atn(atn* node, atn* new,int* height){
	int 	i;
	int 	rh=1,
		lh=1;
	atn* 	temp;
	
	i=avl_data_compare(node->data,new->data);
	if(i==0)
		return 0;
	
	if(i<0){
		//left 
		if(node->left){
			temp=insert_atn(node->left,new,&lh);
			if(temp)
				node->left=temp;
			else
				return 0;
		}
		else
			node->left=new;
		rh=get_height(node->right);			
	}
	else{
		//right
		if(node->right){
			temp=insert_atn(node->right,new,&rh);
			if(temp)
				node->right=temp;
			else
				return 0;
		}
		else
			node->right=new;
		lh=get_height(node->left);
	}
		
	i=lh-rh;
	//printf("lh=%d rh=%d\n",lh,rh);
	if(i==2 || i==-2){
		node=reconstruct(node,lh,rh);
		*height=get_height(node);
	}
	else
		*height=1+max(rh,lh);
		
	return node;
}

atn* avl_tree_insert(atn* node, avl_data_type data){
	atn*	temp;
	atn*	new;
	int 	height;
	
	new=malloc(sizeof(atn));
	new->left=0;
	new->right=0;
	//new->parent=0;
	new->data=data;
	
	if(!node)
		return new;
	
	temp=insert_atn(node,new,&height);
	
	if(temp==0)
		free(new);
	return temp;
}

void avl_tree_free(atn* t){
	if(t){
		if(t->data)
			avl_data_free(t->data);
		if(t->left)
			avl_tree_free(t->left);
		if(t->right)
			avl_tree_free(t->right);
		//printk("<0>" "avl_tree_free with index :%d\n",t->data->index);
		free(t);
	}
	return;
}

void print_avl_tree(atn* t){
	int 	i=-1,
		j=-1;
	
	if(t){
		//printf("%lx %lx\n",t->left,t->right);
		if(t->left)
			i=t->left->data->index;
		if(t->right)
			j=t->right->data->index;
		printf("%d : %d %d\n",t->data->index,i,j);
		print_avl_tree(t->left);
		print_avl_tree(t->right);
	}
	return;
}

int avl_tree_get_size(atn* t){
	if(t)
		return 1+avl_tree_get_size(t->left)+avl_tree_get_size(t->right);
	return 0;
}

void avl_tree_add_node_to_array(atn* t, atn** res, int* index){
	if(t->left)
		avl_tree_add_node_to_array(t->left, res, index);
	res[(*index)++]=t;
	if(t->right)
		avl_tree_add_node_to_array(t->right, res, index);
	return;
}

int avl_tree_get_all_nodes(atn* root,atn** res,int res_len){
	int i=0;
	
	if( root==0 || res_len < avl_tree_get_size(root) )
		return -1;
	avl_tree_add_node_to_array(root, res, &i);
	return i;
}

/*
void main(){
	atn* a=avl_tree_insert(0,10);
	print_avl_tree(a);
	
	a=avl_tree_insert(a,9);
	printf("\n\****\n",a);
	print_avl_tree(a);
	
	a=avl_tree_insert(a,6);
	printf("\n****\n",a);
	print_avl_tree(a);
	
	a=avl_tree_insert(a,4);
	printf("\n****\n",a);
	print_avl_tree(a);
	
	a=avl_tree_insert(a,5);
	printf("\n****\n",a);
	print_avl_tree(a);
	
	a=avl_tree_insert(a,50);
	printf("\n****\n",a);
	print_avl_tree(a);
	
	return;
}
*/