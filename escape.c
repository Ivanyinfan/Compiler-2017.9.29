#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"

typedef struct VarEntry_ *VarEntry;
struct VarEntry_
{
	int level;
	A_dec dec;
};

static void transExp(A_exp exp,int level,S_table vtable);
static void transVar(A_var v,int level,S_table vtable);
static void transDec(A_dec d,int level,S_table vtable);

//书中99页，6.2.3
void Esc_findEscape(A_exp exp) {
	//your code here
	//fprintf(stdout,"[escape][Esc_findEscape] begin\n");fflush(stdout);
	S_table vtable=S_empty();
	transExp(exp,0,vtable);
	//fprintf(stdout,"[escape][Esc_findEscape] complete\n");fflush(stdout);
}

static void transExp(A_exp exp,int level,S_table vtable)
{
	switch(exp->kind)
	{
		case A_varExp://0
			//fprintf(stdout,"[escape][transExp] A_varExp\n");fflush(stdout);
			transVar(exp->u.var,level,vtable);
			break;
		case A_callExp://4
			//fprintf(stdout,"[escape][transExp] A_callExp\n");fflush(stdout);
			for(A_expList args=exp->u.call.args;args;args=args->tail)
				transExp(args->head,level,vtable);
			break;
		case A_opExp://5
			//fprintf(stdout,"[escape][transExp] A_opExp\n");fflush(stdout);
			transExp(exp->u.op.left,level,vtable);
			transExp(exp->u.op.right,level,vtable);
			break;
		case A_seqExp://7
			//fprintf(stdout,"[escape][transExp] A_seqExp\n");fflush(stdout);
			for(A_expList e=exp->u.seq;e;e=e->tail)
				transExp(e->head,level,vtable);
			break;
		case A_assignExp://8
			//fprintf(stdout,"[escape][transExp] A_assignExp\n");fflush(stdout);
			transVar(exp->u.assign.var,level,vtable);
			transExp(exp->u.assign.exp,level,vtable);
			break;
		case A_ifExp://9
			//fprintf(stdout,"[escape][transExp] A_ifExp\n");fflush(stdout);
			transExp(exp->u.iff.test,level,vtable);
			transExp(exp->u.iff.then,level,vtable);
			if(exp->u.iff.elsee)
				transExp(exp->u.iff.elsee,level,vtable);
			break;
		case A_forExp://11
			//fprintf(stdout,"[escape][transExp] A_forExp\n");fflush(stdout);
			transExp(exp->u.forr.lo,level,vtable);
			//fprintf(stdout,"[escape][transExp] trans lo complete\n");fflush(stdout);
			transExp(exp->u.forr.hi,level,vtable);
			//fprintf(stdout,"[escape][transExp] trans hi complete\n");fflush(stdout);
			transExp(exp->u.forr.body,level,vtable);
			break;
		case A_letExp://13
			//fprintf(stdout,"[escape][transExp] A_letExp\n");fflush(stdout);
			S_beginScope(vtable);
			for (A_decList decs=exp->u.let.decs;decs;decs=decs->tail)
            	transDec(decs->head,level,vtable);
            transExp(exp->u.let.body,level,vtable);
            S_endScope(vtable);
            break;
	}
}

static void transVar(A_var v,int level,S_table vtable)
{
	//fprintf(stdout,"[escape][transVar] level=%d\n",level);fflush(stdout);
	switch(v->kind)
	{
		case A_simpleVar:
		{
			//fprintf(stdout,"[escape][transVar] A_simpleVar %s\n",S_name(v->u.simple));fflush(stdout);
			VarEntry ve=S_look(vtable,v->u.simple);
			if(ve&&ve->level<level)
			{
				//fprintf(stdout,"[escape]find %s escape\n",S_name(v->u.simple));fflush(stdout);
				ve->dec->u.var.escape=TRUE;
			}
			break;
		}
		case A_fieldVar:
		{
			//fprintf(stdout,"[escape][transVar] A_fieldVar %s\n",S_name(v->u.field.sym));fflush(stdout);
			transVar(v->u.field.var,level,vtable);
			break;
		}
		case A_subscriptVar:
		{
			//fprintf(stdout,"[escape][transVar] A_subscriptVar\n");fflush(stdout);
			transVar(v->u.subscript.var,level,vtable);
			transExp(v->u.subscript.exp,level,vtable);
			break;
		}
	}
}

static void transDec(A_dec d,int level,S_table vtable)
{
	switch(d->kind)
	{
		case A_functionDec:
		{
			//fprintf(stdout,"[escape][transDec] A_functionDec\n");fflush(stdout);
			for(A_fundecList i=d->u.function;i;i=i->tail)
			{
				A_exp body=i->head->body;
				transExp(body,level+1,vtable);
			}
			break;
		}
		case A_varDec:
		{
			//fprintf(stdout,"[escape][transDec] A_varDec\n");fflush(stdout);
			transExp(d->u.var.init,level,vtable);
			VarEntry ve=checked_malloc(sizeof(*ve));
			ve->level=level;ve->dec=d;
			//fprintf(stdout,"[escape][transDec] %s at %d\n",S_name(d->u.var.var),level);fflush(stdout);
			S_enter(vtable,d->u.var.var,ve);
			break;
		}
	}
}
