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

struct Tr_access_ {
	Tr_level level;
	F_access access;
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

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

struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};

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
                     T_Eseq(T_Label(t),
                             T_Temp(r))))));
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
    l->frame = F_newFrame(Temp_newlabel(), NULL);
    return l;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals)
{
	Tr_level l = checked_malloc(sizeof(*l));
    l->parent = parent;
    l->frame = F_newFrame(name, U_BoolList(TRUE, formals));
    return l;
}

Tr_accessList Tr_formals(Tr_level level)
{
	F_accessList formals=level->frame->formals;
	return Tr_AccessList2(level,formals);
}

Tr_access Tr_allocLocal(Tr_level level, bool escape)
{
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}

Tr_exp Tr_Nil()
{
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_Int(int x)
{
	return Tr_Ex(T_Const(x));
}

Tr_exp Tr_String(string s)
{
    Temp_label l = Temp_newlabel();
    F_frag frag = F_StringFrag(l,s);
    fragList = F_FragList(frag, fragList);
    return Tr_Ex(T_Name(l));
}

Tr_exp Tr_Call(Temp_label label,Tr_expList args,Tr_level level,Tr_level cur)
{
	//fprintf(stderr,"[Tr_call] begin\n");
	T_exp e1 = staticLink(cur,level->parent);
	//fprintf(stderr,"[Tr_call] staticLink return\n");
	T_expList el = T_ExpList2(args);
	//fprintf(stderr,"Tr_call\n");
    return Tr_Ex(T_Call(T_Name(label),T_ExpList(e1,el)));
}

Tr_exp Tr_Op(A_oper oper,Tr_exp left,Tr_exp right,bool isString)
{
	T_exp leftt,rightt;
	struct Cx cx;
	if (isString)
	{
        leftt=F_externalCall("StrCmp",T_ExpList(unEx(left),T_ExpList(unEx(right),NULL)));
        rightt = T_Const(0);
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
	Temp_temp t = Temp_newtemp();
	T_exp texp=F_externalCall("Malloc",T_ExpList(T_Const(num*F_wordSize),NULL));
    return Tr_Ex(T_Eseq(T_Move(T_Temp(t),texp),T_Eseq(Fields(t, num - 1, fields),T_Temp(t))));
}

Tr_exp Tr_Seq(Tr_exp a, Tr_exp b)
{
    return Tr_Ex(T_Eseq(unNx(a), unEx(b)));
}

Tr_exp Tr_Assign(Tr_exp dst, Tr_exp src)
{
    return Tr_Nx(T_Move(unEx(dst), unEx(src)));
}

Tr_exp Tr_If(Tr_exp test, Tr_exp then, Tr_exp elsee)
{
	Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    Temp_temp r = Temp_newtemp();
    Temp_label done = Temp_newlabel();
    struct Cx cx = unCx(test);
    doPatch(cx.trues, t);
    doPatch(cx.falses, f);
    //fprintf(stderr,"Tr_If\n");
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
	Temp_label t = Temp_newlabel();
    Temp_label b = Temp_newlabel();
    struct Cx cx = unCx(test);
    doPatch(cx.trues, b);
    doPatch(cx.falses, done);
    return Tr_Nx(T_Seq(T_Label(t),
                T_Seq(cx.stm,
                    T_Seq(T_Label(b),
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
	Temp_temp r = Temp_newtemp();
    Temp_temp i = Temp_newtemp();
    Temp_temp limit = Temp_newtemp();
    Temp_temp v = Temp_newtemp();
    Temp_label t = Temp_newlabel();
    Temp_label b = Temp_newlabel();
    Temp_label done = Temp_newlabel();
    return Tr_Ex(T_Eseq(T_Move(T_Temp(limit), unEx(size)),
                T_Eseq(T_Move(T_Temp(v), unEx(init)),
                        T_Eseq(T_Move(T_Temp(r), F_externalCall("Malloc", T_ExpList(T_Binop(T_mul, T_Temp(limit), T_Const(F_wordSize)), NULL))),
                            T_Eseq(T_Move(T_Temp(i), T_Const(0)),
                                T_Eseq(T_Label(t),
                                    T_Eseq(T_Cjump(T_lt, T_Temp(i), T_Temp(limit), b, done),
                                        T_Eseq(T_Label(b),
                                            T_Eseq(T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Binop(T_mul, T_Temp(i), T_Const(F_wordSize)))), T_Temp(v)),
                                                T_Eseq(T_Move(T_Temp(i), T_Binop(T_plus, T_Temp(i), T_Const(1))),
                                                    T_Eseq(T_Jump(T_Name(t), Temp_LabelList(t, NULL)),
                                                        T_Eseq(T_Label(done),
                                                            T_Temp(r)))))))))))));
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level)
{
    T_exp texp = staticLink(level, access->level);
    return Tr_Ex(F_Exp(access->access, texp));
}

Tr_exp Tr_fieldVar(Tr_exp addr, int index)
{
    return Tr_Ex(T_Mem(T_Binop(T_plus,unEx(addr),T_Const(index*F_wordSize))));
}

Tr_exp Tr_subscriptVar(Tr_exp addr, Tr_exp index)
{
	return Tr_Ex(T_Mem(T_Binop(T_plus,unEx(addr),T_Binop(T_mul, unEx(index), T_Const(F_wordSize)))));
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals)
{
	//fprintf(stderr,"[Tr_procEntryExit]\n");
	F_frag frag = F_ProcFrag(NULL, NULL);
    fragList = F_FragList(frag, fragList);
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
	//fprintf(stderr,"[staticLink]\n");
    T_exp texp = T_Temp(F_FP());
    Tr_level tmp;int i=0;
    //if(a)fprintf(stderr,"erra\n");
    if(!b)b=Tr_outermost();
    for(tmp=a;tmp&&tmp!=b;tmp=tmp->parent)
    {//if(!tmp->frame)
    //fprintf(stderr,"err\n");++i;
    //fprintf(stderr,"%d\n",i);
    F_accessList formals=F_formals(tmp->frame);
    if(formals)texp = F_Exp(F_formals(tmp->frame)->head, texp);
    else texp=T_Const(0);}
    //fprintf(stderr,"complete\n");
    return texp;
}

static T_expList T_ExpList2(Tr_expList args)
{
	if(!args)
		return NULL;
	return T_ExpList(unEx(args->head),T_ExpList2(args->tail));
}

static T_stm Fields(Temp_temp r, int num, Tr_expList fields)
{
	if(!fields)
		return T_Exp(T_Const(0));
	return T_Seq(T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Const(F_wordSize * num))), unEx(fields->head)),
                Fields(r, num - 1, fields->tail));
}
