#include <console/bptree.h>
#include <stdlib.h>

int bp_search(BpTree * bp_tree, uint32_t bp)
{
    BpTree * search = bp_tree;
    while(search != NULL)
    {
        if(search->bp == bp)
            return 1;
        else if(search->bp > bp)
            search = (BpTree *)search->left;
        else
            search = (BpTree *)search->right;
    }
    return 0;
}

int bp_add(BpTree ** p_bp_tree, uint32_t bp)
{
    BpTree ** search = p_bp_tree;
    while(*search != NULL)
    {
        if((*search)->bp == bp)
            return 0;
        else if((*search)->bp > bp)
            search = (BpTree **)&((*search)->left);
        else
            search = (BpTree **)&((*search)->right);
    }
    *search = calloc(sizeof(BpTree), 1);
    (*search)->bp= bp;
    return 1;
}

int bp_del(BpTree ** p_bp_tree, uint32_t bp)
{
    BpTree ** parent = p_bp_tree;
    BpTree * search = *p_bp_tree;
    
    /* delete root */
    if(search->bp == bp)
    {
        *parent = search->left;
        while((*parent)->right != NULL)
            parent = (BpTree **)&((*parent)->right);
        (*parent)->right = search->right;
        free(search);
        return 0;
    }
    else if(search->bp > bp)
        search = (BpTree *)(search->left);
    else
        search = (BpTree *)(search->right);
    
    while(search != NULL)
    {
        if(search->bp == bp)
        {
            if((*parent)->left == search)
                (*parent)->left = search->left;
            else
                (*parent)->right = search->left;
            while((*parent)->right != NULL)
                parent = (BpTree **)&((*parent)->right);
            (*parent)->right = search->right;
            free(search);
            return 0;
        }
        else if(search->bp > bp)
        {
            parent = (BpTree **)&((*parent)->left);
            search = (BpTree *)(search->left);
        }
        else
        {
            parent = (BpTree **)&((*parent)->right);
            search = (BpTree *)(search->right);
        }
    }
    
    return 1;
}
