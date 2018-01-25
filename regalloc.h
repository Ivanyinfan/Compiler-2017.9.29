/* function prototype from regalloc.c */

#ifndef REGALLOC_H
#define REGALLOC_H

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

struct RA_result {Temp_map coloring; AS_instrList il;};
struct RA_result RA_regAlloc(F_frame f, AS_instrList il);

static void Build(struct Live_graph g);
static void MakeWorklist();
static void Simplify();
static G_nodeList Adjacent(G_node n);
static void DecrementDegree(G_node n);
static void EnableMoves(G_nodeList nodes);
static Live_moveList NodeMoves(G_node n);
static bool MoveRelated(G_node n);
static void Coalesce();
static G_node GetAlias(G_node n);
static void AddWorklist(G_node u);
static bool precolored(G_node n);
static bool OK(G_node v, G_node u);
static bool Conservative(G_nodeList nodes);
static void Combine(G_node u, G_node v);
static void AddEdge(G_node u, G_node v);
static void Freeze();
static void FreezeMoves(G_node u);
static void SelectSpill();
static void AssignColors();
static void RewriteProgram(F_frame f, AS_instrList *pil);
static Temp_map generate_map();

static bool Live_inMoveList(G_node src, G_node dst, Live_moveList l);
static Live_moveList Live_UnionMoveList(Live_moveList l, Live_moveList r);
static Live_moveList Live_SubMoveList(Live_moveList l, Live_moveList r);
G_nodeList G_UnionNodeList(G_nodeList l, G_nodeList r);
G_nodeList G_SubNodeList(G_nodeList l, G_nodeList r);

Temp_tempList* Inst_def(AS_instr inst);
Temp_tempList* Inst_use(AS_instr inst);
#endif
