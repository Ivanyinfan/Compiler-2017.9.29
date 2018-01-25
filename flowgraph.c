#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

/* 控制流图 */

/* 节点n中定值的变量组成的表 */
Temp_tempList FG_def(G_node n) {
	//your code here.
	AS_instr inst = G_nodeInfo(n);
    switch (inst->kind)
    {
        case I_OPER:return inst->u.OPER.dst;
        case I_LABEL:return NULL;
        case I_MOVE:return inst->u.MOVE.dst;
    }
    return NULL;
}

/* 节点n中使用的临时变量组成的表 */
Temp_tempList FG_use(G_node n) {
	//your code here.
	AS_instr inst = G_nodeInfo(n);
    switch (inst->kind)
    {
        case I_OPER:return inst->u.OPER.src;
        case I_LABEL:return NULL;
        case I_MOVE:return inst->u.MOVE.src;
    }
	return NULL;
}

/* 指明n表示的指令是否是move指令，如果是，def和use相同时可以删除 */
bool FG_isMove(G_node n) {
	//your code here.
	AS_instr inst = G_nodeInfo(n);
    return inst->kind == I_MOVE;
}

/* 根据指令表产生控制流图 */
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
	//your code here.
	TAB_table instruction = TAB_empty();
	TAB_table label = TAB_empty();
    G_graph ret = G_Graph();
    G_node last = NULL;
    for (AS_instrList i = il; i; i = i->tail)
    {
        G_node node = G_Node(ret, i->head);
        TAB_enter(instruction, i->head, node);
        if (last)
            G_addEdge(last, node);
        last = node;
        if (i->head->kind == I_LABEL)
            TAB_enter(label, i->head->u.LABEL.label, node);
    }
    for (AS_instrList i = il; i; i = i->tail)
    {
        if (i->head->kind == I_OPER)
        {
            G_node node = TAB_look(instruction, i->head);
            Temp_labelList labels = i->head->u.OPER.jumps->labels;
            for (; labels; labels = labels->tail)
                G_addEdge(node, TAB_look(label, labels->head));
        }
    }
    return ret;
}
