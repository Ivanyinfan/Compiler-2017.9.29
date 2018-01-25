#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: put your code here
#define MAXLINE 40

static AS_instrList iList = NULL, last = NULL;

static void emit(AS_instr inst)
{
	//fprintf(stdout,"[codegen][emit] inst %d:\n",inst->key,AssemInst(inst));printASInstr(stdout,inst);fflush(stdout);
    if (last != NULL)
        last = last->tail = AS_InstrList(inst, NULL);
    else
        last = iList = AS_InstrList(inst, NULL);
}

static Temp_tempList L(Temp_temp h, Temp_tempList t)
{
    return Temp_TempList(h, t);
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
	AS_instrList list; T_stmList sl;
	for (sl=stmList; sl; sl = sl->tail)
        munchStm(sl->head);
    list = iList;iList = NULL;last = NULL;
    list=F_procEntryExit2(list);
    return list;
}

static void munchStm(T_stm s)
{
	switch(s->kind)
	{
		case T_MOVE:
		{
			T_exp dst=s->u.MOVE.dst,src=s->u.MOVE.src;
			//1
			if(dst->kind == T_TEMP && src->kind == T_MEM && src->u.MEM->kind == T_BINOP && src->u.MEM->u.BINOP.op == T_plus && src->u.MEM->u.BINOP.right->kind == T_CONST)
			{
				Temp_temp left = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.left);
				int off = s->u.MOVE.src->u.MEM->u.BINOP.right->u.CONST;
        		char *assem = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(assem, "movl %d(`s0), `d0", off);
        		emit(AS_Oper(assem, L(dst->u.TEMP, NULL), L(left, NULL), AS_Targets(NULL)));
        	}
        	//2
        	else if(dst->kind == T_TEMP && src->kind == T_MEM && src->u.MEM->kind == T_BINOP && src->u.MEM->u.BINOP.op == T_plus && src->u.MEM->u.BINOP.left->kind == T_CONST)
        	{
        		Temp_temp right = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.right);
        		int off = s->u.MOVE.src->u.MEM->u.BINOP.left->u.CONST;
        		char *assem = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(assem, "movl %d(`s0), `d0", off);
        		emit(AS_Oper(assem, L(dst->u.TEMP, NULL), L(right, NULL), AS_Targets(NULL)));
        	}
        	//3
        	else if(dst->kind == T_MEM && dst->u.MEM->kind == T_BINOP && dst->u.MEM->u.BINOP.op == T_plus && dst->u.MEM->u.BINOP.right->kind == T_CONST)
        	{
        		Temp_temp msrc = munchExp(s->u.MOVE.src);
		        Temp_temp mleft = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.left);
        		int off = s->u.MOVE.dst->u.MEM->u.BINOP.right->u.CONST;
        		char *assem = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(assem, "movl `s0, %d(`s1)", off);
        		emit(AS_Oper(assem, NULL, L(msrc, L(mleft, NULL)), AS_Targets(NULL)));
        	}
        	//4
        	else if(dst->kind == T_MEM && dst->u.MEM->kind == T_BINOP && dst->u.MEM->u.BINOP.op == T_plus && dst->u.MEM->u.BINOP.left->kind == T_CONST)
        	{
        		Temp_temp msrc = munchExp(s->u.MOVE.src);
        		Temp_temp mright = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.right);
        		int off = s->u.MOVE.dst->u.MEM->u.BINOP.left->u.CONST;
        		char *a = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(a, "movl `s0, %d(`s1)", off);
        		emit(AS_Oper(a, NULL, L(msrc, L(mright, NULL)), AS_Targets(NULL)));
        	}
        	//5
        	else if(dst->kind == T_TEMP && src->kind == T_TEMP)
        	{
				emit(AS_Move("movl `s0, `d0", L(dst->u.TEMP, NULL), L(src->u.TEMP, NULL)));
			}
			//6
			else if(dst->kind == T_TEMP)
			{
				Temp_temp msrc = munchExp(s->u.MOVE.src);
        		emit(AS_Move("movl `s0, `d0", L(dst->u.TEMP, NULL), L(msrc, NULL)));
        	}
        	//7
        	else if(dst->kind == T_MEM)
        	{
        		Temp_temp msrc = munchExp(s->u.MOVE.src);
        		Temp_temp mdst = munchExp(s->u.MOVE.dst->u.MEM);
        		emit(AS_Oper("movl `s0, (`s1)", NULL, L(msrc, L(mdst, NULL)), AS_Targets(NULL)));
        	}
        	break;
        }
        case T_JUMP:
        {
        	T_exp exp=s->u.JUMP.exp;
        	//8
        	if(exp->kind == T_NAME)
        	{
        		Temp_label name = s->u.JUMP.exp->u.NAME;
        		char *assem = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(assem, "jmp %s", Temp_labelstring(name));
        		emit(AS_Oper(assem, NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
        	}
        	//9
        	else
        	{
        		Temp_temp mexp = munchExp(exp);
        		emit(AS_Oper("jmp *`s0", NULL, L(mexp, NULL), AS_Targets(s->u.JUMP.jumps)));
        	}
        	break;
        }
        case T_CJUMP:
        {
        	Temp_temp left = munchExp(s->u.CJUMP.left);
        	Temp_temp right = munchExp(s->u.CJUMP.right);
        	switch(s->u.CJUMP.op)
        	{
        		//10
        		case T_eq:
        		{
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			char *assem = checked_malloc(MAXLINE * sizeof(char));
        			sprintf(assem, "je %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(assem, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        		//11
        		case T_ne:
        		{
        			char *assem = checked_malloc(MAXLINE * sizeof(char));
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			sprintf(assem, "jne %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(assem, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        		//12
        		case T_lt:
        		{
        			char *assem = checked_malloc(MAXLINE * sizeof(char));
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			sprintf(assem, "jl %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(assem, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        		//13
        		case T_gt:
        		{
        			char *a = checked_malloc(MAXLINE * sizeof(char));
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			sprintf(a, "jg %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(a, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        		//14
        		case T_le:
        		{
        			char *a = checked_malloc(MAXLINE * sizeof(char));
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			sprintf(a, "jle %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(a, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        		//15
        		case T_ge:
        		{
        			char *a = checked_malloc(MAXLINE * sizeof(char));
        			emit(AS_Oper("cmp `s0, `s1", NULL, L(right, L(left, NULL)), AS_Targets(NULL)));
        			sprintf(a, "jge %s", Temp_labelstring(s->u.CJUMP.true));
        			emit(AS_Oper(a, NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
        			break;
        		}
        	}
        	break;
        }
        case T_LABEL:
        {
        	char *a = checked_malloc(MAXLINE * sizeof(char));
        	sprintf(a, "%s", Temp_labelstring(s->u.LABEL));
        	emit(AS_Label(a, s->u.LABEL));
        	break;
        }
        case T_SEQ:
        {
        	munchStm(s->u.SEQ.left);
        	munchStm(s->u.SEQ.right);
        	break;
        }
        case T_EXP:
        {
        	munchExp(s->u.EXP);
        	break;
        }
	}
}

static Temp_temp munchExp(T_exp e)
{
	Temp_temp ret = Temp_newtemp();
	switch(e->kind)
	{
		case T_MEM:
		{
			T_exp MEM=e->u.MEM;
			//1
			if(MEM->kind == T_BINOP && MEM->u.BINOP.op == T_plus && MEM->u.BINOP.right->kind == T_CONST)
			{
				Temp_temp left = munchExp(e->u.MEM->u.BINOP.left);
        		int CONST = e->u.MEM->u.BINOP.right->u.CONST;
        		char *a = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(a, "movl %d(`s0), `d0", CONST);
        		emit(AS_Oper(a, L(ret, NULL), L(left, NULL), AS_Targets(NULL)));
        	}
        	//2
        	else if(MEM->kind == T_BINOP && e->u.MEM->u.BINOP.op == T_plus && e->u.MEM->u.BINOP.left->kind == T_CONST)
        	{
        		Temp_temp right = munchExp(e->u.MEM->u.BINOP.right);
        		int CONST = e->u.MEM->u.BINOP.left->u.CONST;
        		char *a = checked_malloc(MAXLINE * sizeof(char));
        		sprintf(a, "movl %d(`s0), `d0", CONST);
        		emit(AS_Oper(a, L(ret, NULL), L(right, NULL), AS_Targets(NULL)));
        	}
        	else
        	{
        		Temp_temp MEM = munchExp(e->u.MEM);
        		emit(AS_Oper("movl (`s0), `d0", L(ret, NULL), L(MEM, NULL), AS_Targets(NULL)));
        	}
        	break;
        }
        case T_CALL:
        {
        	//3
        	if(e->u.CALL.fun->kind == T_NAME)
        	{
        		Temp_temp rv = F_RV();
        		Temp_label fun = e->u.CALL.fun->u.NAME;
        		char *a = checked_malloc(MAXLINE * sizeof(char));
        		munchArgs(e->u.CALL.args);
        		sprintf(a, "call %s", Temp_labelstring(fun));
        		emit(AS_Oper(a, L(rv, L(F_ecx(), L(F_edx(), NULL))), NULL, AS_Targets(NULL)));
        		emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(rv, NULL)));
        	}
        	//4
        	else
        	{
        		Temp_temp rv = F_RV();
        		Temp_temp s = munchExp(e->u.CALL.fun);
        		munchArgs(e->u.CALL.args);
        		emit(AS_Oper("call *`s0", L(rv, L(F_ecx(), L(F_edx(), NULL))), L(s, NULL), AS_Targets(NULL)));
        		emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(rv, NULL)));
        	}
        	break;
        }
        case T_BINOP:
        {
        	Temp_temp left = munchExp(e->u.BINOP.left);
        	Temp_temp right = munchExp(e->u.BINOP.right);
        	switch(e->u.BINOP.op)
        	{
        		case T_plus:
        		{
        			emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(left, NULL)));
        			emit(AS_Oper("addl `s0, `d0", L(ret, NULL), L(right, L(ret, NULL)), AS_Targets(NULL)));
        			break;
        		}
        		case T_minus:
        		{
        			emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(left, NULL)));
        			emit(AS_Oper("subl `s0, `d0", L(ret, NULL), L(right, L(ret, NULL)), AS_Targets(NULL)));
        			break;
        		}
        		case T_mul:
        		{
        			emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(left, NULL)));
        			emit(AS_Oper("imul `s0, `d0", L(ret, NULL), L(right, L(ret, NULL)), AS_Targets(NULL)));
        			break;
        		}
        		case T_div:
        		{
        			Temp_temp edx = F_edx();
        			Temp_temp eax = F_eax();
        			emit(AS_Move("movl `s0, `d0", L(eax, NULL), L(left, NULL)));
					emit(AS_Oper("cltd", L(edx, L(eax, NULL)), L(eax, NULL), AS_Targets(NULL)));
        			emit(AS_Oper("idivl `s0", L(edx, L(eax, NULL)), L(right, L(edx, L(eax, NULL))), AS_Targets(NULL)));
        			emit(AS_Move("movl `s0, `d0", L(ret, NULL), L(eax, NULL)));
        			break;
        		}
        	}
        	break;
        }
        case T_CONST:
        {
        	char *a = checked_malloc(MAXLINE * sizeof(char));
        	sprintf(a, "movl $%d, `d0", e->u.CONST);
        	emit(AS_Oper(a, L(ret, NULL), NULL, AS_Targets(NULL)));
        	break;
        }
        case T_NAME:
        {
        	char *a = checked_malloc(MAXLINE * sizeof(char));
        	sprintf(a, "movl $%s, `d0", Temp_labelstring(e->u.NAME));
        	emit(AS_Oper(a, L(ret, NULL), NULL, AS_Targets(NULL)));
        	break;
        }
        case T_ESEQ:
        {
        	munchStm(e->u.ESEQ.stm);
        	ret=munchExp(e->u.ESEQ.exp);
        	break;
        }
        case T_TEMP:ret=e->u.TEMP;break;
	}
	return ret;
}

static void munchArgs(T_expList args)
{
    if (args)
    {
        munchArgs(args->tail);
        Temp_temp s = munchExp(args->head);
        emit(AS_Oper("pushl `s0", NULL, L(s, NULL), AS_Targets(NULL)));
    }
}
