#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"
#include "assem.h" /* for debug purpose */

static int ltimes;

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

Temp_temp Live_gtemp(G_node n) {
	//your code here.
	return G_nodeInfo(n);
}

G_node allocNode(G_graph g, Temp_temp temp, TAB_table temp2node)
{
    G_node res = TAB_look(temp2node, temp);
    if (!res) {
        res = G_Node(g, temp);
        TAB_enter(temp2node, temp, res);
    }
    return res;
}

bool * G_adjSet(bool * set, int cnt, int i, int j)
{
    return set + (j + cnt * i);
}

void addEdge(struct Live_graph *g, Temp_temp temp_a, Temp_temp temp_b, TAB_table temp2node, G_table spillCost)
{
    if (temp_a == temp_b || temp_a == F_FP() || temp_b == F_FP()) return;

    G_node a = allocNode(g->graph, temp_a, temp2node);
    G_node b = allocNode(g->graph, temp_b, temp2node);
    if (!Temp_inTempList(temp_a, F_registers()))
    {
        int *r = G_look(spillCost, a);
        ++(*r);
    }
    if (!Temp_inTempList(temp_b, F_registers()))
    {
        int *r = G_look(spillCost, b);
        ++(*r);
    }
	
    bool * cell = G_adjSet(g->adj, G_getNodecount(g->graph), G_getMykey(a), G_getMykey(b));
    if (!*cell) {
        *cell = TRUE;
        cell = G_adjSet(g->adj, G_getNodecount(g->graph), G_getMykey(b), G_getMykey(a));
        *cell = TRUE;
        G_addEdge(a, b);
        G_addEdge(b, a);
    }
}

struct Live_graph Live_liveness(G_graph flow,int times) {
	ltimes=times;
	struct Live_graph lg;
    G_table in = G_empty();
    G_table out = G_empty();
    TAB_table temp2node = TAB_empty();
    G_nodeList p;
    for (p = G_nodes(flow); p; p = p->tail) {
        G_enter(in, p->head, checked_malloc(sizeof(Temp_tempList*)));
        G_enter(out, p->head, checked_malloc(sizeof(Temp_tempList*)));
    }
    bool change = TRUE;
    while (change)
    {
        change = FALSE;
        p = G_nodes(flow);
        for (; p; p = p->tail)
		{
			AS_instr inst=G_nodeInfo(p->head);
            Temp_tempList inp0 = *(Temp_tempList*)G_look(in, p->head);
            Temp_tempList outp0 = *(Temp_tempList*)G_look(out, p->head);
            Temp_tempList inp, outp;
            G_nodeList succ = G_succ(p->head);
            outp = NULL;
            for (; succ ; succ = succ->tail)
			{
				AS_instr succInst=G_nodeInfo(succ->head);
                Temp_tempList ins = *(Temp_tempList*)G_look(in, succ->head);
                outp = Temp_UnionTempList(outp, ins);
            }
            inp = Temp_UnionTempList(FG_use(p->head), Temp_SubTempList(outp, FG_def(p->head)));
            if (!Temp_TempListEqual(inp, inp0))
            {
                change = TRUE;
                *(Temp_tempList*)G_look(in, p->head) = inp;
            }
            if (!Temp_TempListEqual(outp, outp0))
            {
                change = TRUE;
                *(Temp_tempList*)G_look(out, p->head) = outp;
            }
        }
    }
    for(G_nodeList n=G_nodes(flow);n;n=n->tail)
    {
    	AS_instr inst=G_nodeInfo(n->head);
    	Temp_tempList inn = *(Temp_tempList*)G_look(in, n->head);
        Temp_tempList outn = *(Temp_tempList*)G_look(out, n->head);
    }
    
    lg.moves = NULL;
    lg.graph = G_Graph();
    lg.spillCost = G_empty();

    for (Temp_tempList m = F_registers(); m; m = m->tail)
    {
        allocNode(lg.graph, m->head, temp2node);
    }
    
    for (p = G_nodes(flow); p; p = p->tail)
	{
        for (Temp_tempList def = FG_def(p->head); def; def = def->tail)
		{
            if (def->head != F_FP())
			{
                int * r = checked_malloc(sizeof(int));
                *r = 0;
                G_enter(lg.spillCost, allocNode(lg.graph, def->head, temp2node), r);
            }
        }
    }
    lg.adj = checked_malloc(G_getNodecount(lg.graph) * G_getNodecount(lg.graph) * sizeof(bool));
    for (Temp_tempList m1 = F_registers(); m1; m1 = m1->tail)
	{
        for (Temp_tempList m2 = F_registers(); m2; m2 = m2->tail)
		{
            if (m1->head != m2->head)
			{
                addEdge(&lg, m1->head, m2->head, temp2node, lg.spillCost);
            }
        }
    }
    for (p = G_nodes(flow); p; p = p->tail)
	{
        Temp_tempList outp = *(Temp_tempList*)G_look(out, p->head), op;
        AS_instr inst = G_nodeInfo(p->head);
        if (inst->kind == I_MOVE)
		{
            outp = Temp_SubTempList(outp, FG_use(p->head));
            for (Temp_tempList def = FG_def(p->head); def; def = def->tail)
			{
                for (Temp_tempList use = FG_use(p->head); use; use = use->tail)
				{
                    lg.moves = Live_MoveList(allocNode(lg.graph, use->head, temp2node),
                            allocNode(lg.graph, def->head, temp2node),
                            lg.moves);
                }
            }
        }
        for (Temp_tempList def = FG_def(p->head); def; def = def->tail)
		{
            for (op = outp; op; op = op->tail)
			{
                addEdge(&lg, def->head, op->head, temp2node, lg.spillCost);
            }
        }
    }
	lg.temp2node=temp2node;
	return lg;
}
