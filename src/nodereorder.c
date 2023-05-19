#include <math.h>

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"

static void node_enqueue(uint16_t *qarray, uint32_t *qsize, uint16_t val)
{
    if (*qsize == 0) {
        qarray[0] = val;
    }
    else
    {
        for (int i=*qsize;i>0;i--)
        {
            qarray[i] = qarray[i-1];
        }

        qarray[0] = val;
    }

    *qsize += 1;
}

// assuming it has data
static uint16_t node_dequeue(uint16_t *qarray, uint32_t *qsize)
{
    uint16_t val = qarray[0];

    for (int i=0;i<*qsize-1;i++)
    {
		qarray[i] = qarray[i+1];
	}
	*qsize -= 1;

	return val;
}

static inline void copy_node_data_as_is(void *dn, void *sn, uint16_t dsti, uint16_t srci)
{
    node_t* dstn = ((node_t *)dn); // + (dsti*sizeof(node_t));
    for (int i=0;i<dsti;i++)
        dstn++;

    node_t* srcn = ((node_t *)sn); // + (srci*sizeof(node_t));
    for (int i=0;i<srci;i++)
        srcn++;

    memcpy((void*)dstn, (void*)srcn, 1*sizeof(node_t));
}

void node_reorder(int numn, node_t *np)
{
    uint16_t*    qarray = NULL;
    uint32_t     qsize;

    node_t*      new_nodes = NULL;
    int          numNodesExplored;
    uint16_t     node_index;

    uint16_t*    rewrites = NULL;
	
    if(qarray == NULL)
        qarray = (uint16_t*)Z_Malloc(numn*sizeof(uint16_t),PU_STATIC,0);
    if(qarray == NULL)
        I_Error("no queue space");

    if (rewrites == NULL)
        rewrites = (uint16_t*)Z_Malloc(sizeof(uint16_t)*numn,PU_STATIC,0);
    if(rewrites == NULL)
        I_Error("no rewrite space");

    if (new_nodes == NULL)
        new_nodes = (node_t*)Z_Malloc (numn*sizeof(node_t),PU_LEVEL,0);	
    if(new_nodes == NULL)
        I_Error("no new node space");

    qsize = 0;
    memset(new_nodes, 0,     numn*sizeof(node_t));
    memset(qarray,    0,     numn*sizeof(uint16_t));
    memset(rewrites,  0, (numn+1)*sizeof(uint16_t));

    numNodesExplored = numn-1;
    node_index = numn-1;
    rewrites[node_index] = numNodesExplored;
    copy_node_data_as_is(new_nodes, np, numNodesExplored, node_index);
    numNodesExplored-=1;
    node_enqueue(qarray,&qsize,node_index);

    while (qsize > 0) 
    {
        node_index = node_dequeue(qarray,&qsize);

        node_t* n = (node_t *)&np[node_index];
		
        uint16_t child1_idx = n->children[0];
        uint16_t child0_idx = n->children[1];
        // NF_SUBSECTOR
        if (child0_idx < (uint16_t)numn)
        {
            rewrites[child0_idx] = numNodesExplored;
            copy_node_data_as_is(new_nodes, np, numNodesExplored, child0_idx);
            numNodesExplored-=1;
            node_enqueue(qarray,&qsize,child0_idx);
        }

        if (child1_idx < (uint16_t)numn) 
        {
            rewrites[child1_idx] = numNodesExplored;
            copy_node_data_as_is(new_nodes, np, numNodesExplored, child1_idx);
            numNodesExplored-=1;
            node_enqueue(qarray,&qsize,child1_idx);
        }
    }

#if 1
    // do the rewrites here
    node_t *n = new_nodes;
    for (int i=0;i<numn;n++,i++)
    {
        uint16_t child1_idx = n->children[0];
        uint16_t child0_idx = n->children[1];
        if(child0_idx < (uint16_t)numn)
            n->children[1] = rewrites[child0_idx];
        if(child1_idx < (uint16_t)numn)
            n->children[0] = rewrites[child1_idx];
    }

    Z_Free(qarray);
    Z_Free(rewrites);

    // finally swap new nodes for old
    node_t *old_nodes = nodes;
    nodes = new_nodes;
    Z_Free(old_nodes);
#endif
}