#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

static F_fragList fragList;
static int levels=0;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};
F_access getAccess(Tr_access a){if(a)return a->access;return NULL;}

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
	int key;
};
int getKey(Tr_level level)
{
	if(!level)return -1;
	return level->key;
}

typedef struct patchList_ *patchList;
struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};

struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

int getKind(Tr_exp exp){return exp->kind;}

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};
int TrExpListLen(Tr_expList el)
{
	if(!el)return 0;
	return 1+TrExpListLen(el->tail);
}

static T_expList T_ExpList2(Tr_expList args);
static Tr_accessList Tr_AccessList2(Tr_level level, F_accessList formals);
static T_exp staticLink(Tr_level a, Tr_level b);
static T_stm Fields(Temp_temp r, int num, Tr_expList fields);

static Tr_access Tr_Access(Tr_level level, F_access access)
{
    Tr_access res = checked_malloc(sizeof(*res));
    res->level = level;
    res->access = access;
    return res;
}

static Tr_exp Tr_Ex(T_exp ex)
{
    Tr_exp res = checked_malloc(sizeof(*res));
    res->kind = Tr_ex;
    res->u.ex = ex;
    return res;
}

static Tr_exp Tr_Nx(T_stm nx)
{
    Tr_exp res = checked_malloc(sizeof(*res));
    res->kind = Tr_nx;
    res->u.nx = nx;
    return res;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm)
{
    Tr_exp res = checked_malloc(sizeof(*res));
    res->kind = Tr_cx;
    res->u.cx.trues = trues;
    res->u.cx.falses = falses;
    res->u.cx.stm = stm;
    return res;
}

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

static T_exp unEx(Tr_exp e) {
    switch(e->kind) {
        case Tr_ex:
          return e->u.ex;
        case Tr_cx: {
          Temp_temp r = Temp_newtemp();
          Temp_label t = Temp_newlabel(), f = Temp_newlabel();
          doPatch(e->u.cx.trues, t);
          doPatch(e->u.cx.falses, f);
          return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                  T_Eseq(e->u.cx.stm,
                   T_Eseq(T_Label(f),
                    T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                     T_Eseq(T_Label(t),T_Temp(r))))));
        }
        case Tr_nx:
          return T_Eseq(e->u.nx, T_Const(0));
    }
    assert(0); /* can't get here */
}

static T_stm unNx(Tr_exp e)
{
    switch(e->kind)
    {
        case Tr_ex:
            return T_Exp(e->u.ex);
        case Tr_cx:
        {
        	Temp_label l = Temp_newlabel();
            doPatch(e->u.cx.trues, l);
            doPatch(e->u.cx.falses, l);
            return T_Seq(e->u.cx.stm, T_Label(l));
        }
        case Tr_nx:
            return e->u.nx;
    }
}

static struct Cx unCx(Tr_exp e)
{
    switch(e->kind)
    {
        case Tr_ex:
        {
        	struct Cx res;
			res.stm = T_Cjump(T_ne, unEx(e), T_Const(0), NULL, NULL);
            res.trues = PatchList(&(res.stm->u.CJUMP.true), NULL);
            res.falses = PatchList(&(res.stm->u.CJUMP.false), NULL);
            return res;
        }
        case Tr_cx:
            return e->u.cx;
        case Tr_nx:
            assert(0);
    }
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail)
{
    Tr_expList el = checked_malloc(sizeof(*el));
    el->head = head;
    el->tail = tail;
    return el;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail)
{
	Tr_accessList al = checked_malloc(sizeof(*al));
    al->head = head;
    al->tail = tail;
    return al;
}

Tr_level Tr_outermost(void)
{
	Tr_level l = checked_malloc(sizeof(*l));
    l->parent = NULL;
    l->frame = F_newFrame(Temp_namedlabel("tigermain"), NULL);
    l->key=levels++;
    return l;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals)
{
	Tr_level l = checked_malloc(sizeof(*l));
    l->parent = parent;
    l->frame = F_newFrame(name, U_BoolList(TRUE, formals));
    l->key=levels++;
    return l;
}

Tr_accessList Tr_formals(Tr_level level)
{
	F_accessList formals=level->frame->formals;
	formals=formals->tail;//跳过静态链
	return Tr_AccessList2(level,formals);
}

Tr_access Tr_allocLocal(Tr_level level, bool escape)
{
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}

Tr_exp Tr_Nil()
{
	//////fprintf(stdout,"[translate][Tr_Nil] begin\n");fflush(stdout);
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_Int(int x)
{
	//////fprintf(stdout,"[translate][Tr_Int] begin\n");fflush(stdout);
	return Tr_Ex(T_Const(x));
}

Tr_exp Tr_String(string s)
{
	////fprintf(stdout,"[translate][Tr_String] begin\n");fflush(stdout);
    Temp_label l = Temp_newlabel();
    F_frag frag = F_StringFrag(l,s);
    fragList = F_FragList(frag, fragList);
    return Tr_Ex(T_Name(l));
}

Tr_exp Tr_Call(Temp_label label,Tr_expList args,Tr_level level,Tr_level cur)
{
	//////fprintf(stdout,"[translate][Tr_call] begin\n");fflush(stdout);
	//////fprintf(stdout,"[translate][Tr_call] fun=%s\n",Temp_labelstring(label));fflush(stdout);
	//////fprintf(stdout,"[translate][Tr_call] argsNum=%d\n",TrExpListLen(args));fflush(stdout);
	//////fprintf(stdout,"[translate][Tr_call] level=%d\n",getKey(level));fflush(stdout);
	if(getKey(level)==1)//调用的是系统函数，0表示的main函数
		return Tr_Ex(F_externalCall(Temp_labelstring(label),T_ExpList2(args)));
	T_exp e1 = staticLink(cur,level->parent);
	T_expList el = T_ExpList2(args);
	Tr_exp e=Tr_Ex(T_Call(T_Name(label),T_ExpList(e1,el)));
	//////fprintf(stdout,"[translate][Tr_call] complete\n");fflush(stdout);
    return e;
}

Tr_exp Tr_Op(A_oper oper,Tr_exp left,Tr_exp right,bool isString)
{
	//////fprintf(stdout,"[translate][Tr_Op] begin\n");fflush(stdout);
	T_exp leftt,rightt;
	struct Cx cx;
	if (isString)
	{
        leftt=F_externalCall("stringEqual",T_ExpList(unEx(left),T_ExpList(unEx(right),NULL)));
        rightt = T_Const(1);
    } 
    else
    {
        leftt = unEx(left);
        rightt = unEx(right);
    }
    switch (oper)
    {
        case A_plusOp:
            return Tr_Ex(T_Binop(T_plus, leftt, rightt));
        case A_minusOp:
            return Tr_Ex(T_Binop(T_minus, leftt, rightt));
        case A_timesOp:
            return Tr_Ex(T_Binop(T_mul, leftt, rightt));
        case A_divideOp:
            return Tr_Ex(T_Binop(T_div, leftt, rightt));
        case A_eqOp:
            cx.stm = T_Cjump(T_eq, leftt, rightt, NULL, NULL);
            break;
        case A_neqOp:
            cx.stm = T_Cjump(T_ne, leftt, rightt, NULL, NULL);
            break;
        case A_ltOp:
            cx.stm = T_Cjump(T_lt, leftt, rightt, NULL, NULL);
            break;
        case A_gtOp:
            cx.stm = T_Cjump(T_gt, leftt, rightt, NULL, NULL);
            break;
        case A_leOp:
            cx.stm = T_Cjump(T_le, leftt, rightt, NULL, NULL);
            break;
        case A_geOp:
            cx.stm = T_Cjump(T_ge, leftt, rightt, NULL, NULL);
            break;
    }
    cx.trues = PatchList(&(cx.stm->u.CJUMP.true), NULL);
    cx.falses = PatchList(&(cx.stm->u.CJUMP.false), NULL);
    return Tr_Cx(cx.trues, cx.falses, cx.stm);
}

Tr_exp Tr_Record(int num, Tr_expList fields)
{
	//////fprintf(stdout,"[translate][Tr_Record] begin\n");fflush(stdout);
	Temp_temp t = Temp_newtemp();
	T_exp texp=F_externalCall("allocRecord",T_ExpList(T_Const(num*F_wordSize),NULL));
    Tr_exp ret=Tr_Ex(T_Eseq(T_Move(T_Temp(t),texp),T_Eseq(Fields(t, num - 1, fields),T_Temp(t))));
    //////fprintf(stdout,"[translate][Tr_Record] complete\n");fflush(stdout);
    return ret;
}

Tr_exp Tr_Seq(Tr_exp a, Tr_exp b)
{
	////fprintf(stdout,"[translate][Tr_Seq] begin\n");fflush(stdout);
	//////fprintf(stdout,"[translate][Tr_Seq] a->kind=%d,b->kind=%d\n",a->kind,b->kind);fflush(stdout);
    return Tr_Ex(T_Eseq(unNx(a), unEx(b)));
}

Tr_exp Tr_Assign(Tr_exp dst, Tr_exp src,string from)
{
	//fprintf(stdout,"[translate][Tr_Assign] from %s\n",from);fflush(stdout);
	Tr_exp e=Tr_Nx(T_Move(unEx(dst), unEx(src)));
	//fprintf(stdout,"[translate][Tr_Assign] complete\n");fflush(stdout);
    return e;
}

Tr_exp Tr_If(Tr_exp test, Tr_exp then, Tr_exp elsee)
{
	Temp_label t = Temp_IfTrueLabel();
    Temp_label f = Temp_IfFalseLabel();
    Temp_temp r = Temp_newtemp();
    Temp_label done = Temp_newlabel();
    ////fprintf(stdout,"[translate][Tr_If]test->kind=%d\n",test->kind);fflush(stdout);
    struct Cx cx = unCx(test);
    doPatch(cx.trues, t);
    doPatch(cx.falses, f);
    ////////fprintf(stdout,"Tr_If\n");
    if(elsee)
    	return Tr_Ex(T_Eseq(cx.stm,
        	    T_Eseq(T_Label(t),
            	    T_Eseq(T_Move(T_Temp(r), unEx(then)),
            	        T_Eseq(T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
            	            T_Eseq(T_Label(f),
            	                T_Eseq(T_Move(T_Temp(r), unEx(elsee)),
            	                    T_Eseq(T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
            	                        T_Eseq(T_Label(done), T_Temp(r))))))))));
	return Tr_Nx(T_Seq(cx.stm,
                T_Seq(T_Label(t),
                    T_Seq(unNx(then),
                        T_Label(f)))));
}

Tr_exp Tr_While(Tr_exp test, Tr_exp body, Temp_label done)
{
	Temp_label t = Temp_WhileTestLabel();
    Temp_label l = Temp_WhileLoopLabel();
    struct Cx cx = unCx(test);
    doPatch(cx.trues, l);
    doPatch(cx.falses, done);
    return Tr_Nx(T_Seq(T_Label(t),
                T_Seq(cx.stm,
                    T_Seq(T_Label(l),
                        T_Seq(unNx(body),
                            T_Seq(T_Jump(T_Name(t), Temp_LabelList(t, NULL)),
                                T_Label(done)))))));
}

Tr_exp Tr_For(Tr_access access, Tr_level level, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done)
{
	Tr_exp i = Tr_simpleVar(access, level);
    Temp_temp limit = Temp_newtemp();
    Temp_label b = Temp_newlabel();
    Temp_label inc = Temp_newlabel();
    return Tr_Nx(T_Seq(T_Move(unEx(i), unEx(lo)),
                T_Seq(T_Move(T_Temp(limit), unEx(hi)),
                    T_Seq(T_Cjump(T_gt, unEx(i), T_Temp(limit), done, b),
                        T_Seq(T_Label(b),
                            T_Seq(unNx(body),
                                T_Seq(T_Cjump(T_eq, unEx(i), T_Temp(limit), done, inc),
                                    T_Seq(T_Label(inc),
                                        T_Seq(T_Move(unEx(i), T_Binop(T_plus, unEx(i), T_Const(1))),
                                            T_Seq(T_Jump(T_Name(b), Temp_LabelList(b, NULL)),
                                                T_Label(done)))))))))));
}

Tr_exp Tr_Break(Temp_label done)
{
	return Tr_Nx(T_Jump(T_Name(done), Temp_LabelList(done, NULL)));
}

Tr_exp Tr_Array(Tr_exp size, Tr_exp init)
{
	return Tr_Ex(F_externalCall("initArray", T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level)
{
	////fprintf(stdout,"[translate][Tr_simpleVar] begin\n");fflush(stdout);
    T_exp texp = staticLink(level, access->level);
    Tr_exp e=Tr_Ex(F_Exp(access->access, texp));
    ////fprintf(stdout,"[translate][Tr_simpleVar] complete\n");fflush(stdout);
    return e;
}

Tr_exp Tr_fieldVar(Tr_exp addr, int index)
{
	//////fprintf(stdout,"[translate][Tr_fieldVar] begin\n");fflush(stdout);
    return Tr_Ex(T_Mem(T_Binop(T_plus,unEx(addr),T_Const(index*F_wordSize))));
}

Tr_exp Tr_subscriptVar(Tr_exp addr, Tr_exp index)
{
	return Tr_Ex(T_Mem(T_Binop(T_plus,unEx(addr),T_Binop(T_mul, unEx(index), T_Const(F_wordSize)))));
}

T_stm Tr_procEntryExit1(Tr_level level, Tr_exp body)
{
	return T_Move(T_Temp(F_RV()),unEx(body));
}

void Tr_newProcFrag(Tr_exp body, Tr_level level)
{
	T_stm s=Tr_procEntryExit1(level,body);
	F_frag f=F_ProcFrag(s,level->frame);
	fragList=F_FragList(f,fragList);
}

F_fragList Tr_getResult()
{
	return fragList;
}

/* Auxiliary Function */
static Tr_accessList Tr_AccessList2(Tr_level level, F_accessList formals)
{
	if(!formals)
		return NULL;
	return Tr_AccessList(Tr_Access(level,formals->head),Tr_AccessList2(level,formals->tail));
}

static T_exp staticLink(Tr_level a, Tr_level b)
{
	////fprintf(stdout,"[translate][staticLink] a->key=%d,b->key=%d\n",getKey(a),getKey(b));fflush(stdout);
    T_exp texp = T_Temp(F_FP());
    Tr_level tmp;int i=0;
    for(tmp=a;tmp&&tmp!=b;tmp=tmp->parent)
    {
    	F_accessList formals=F_formals(tmp->frame);
    	if(formals)texp = F_Exp(F_formals(tmp->frame)->head, texp);
    	else texp=T_Const(0);
    	//////fprintf(stdout,"[translate][staticLink] texp\n");printStmList(stdout,T_StmList(T_Exp(texp),NULL));fflush(stdout);
    }
    ////fprintf(stdout,"[translate][staticLink] complete\n");fflush(stdout);
    return texp;
}

//将函数参数反序组成T_expList
static T_expList T_ExpList2(Tr_expList args)
{
	T_expList ret=NULL;
	for(;args;args=args->tail)
		ret=T_ExpList(unEx(args->head),ret);
	return ret;
}

static T_stm Fields(Temp_temp r, int num, Tr_expList fields)
{
	if(!fields)
		return T_Exp(T_Const(0));
	return T_Seq(T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Const(F_wordSize * num))), unEx(fields->head)),
                Fields(r, num - 1, fields->tail));
}
