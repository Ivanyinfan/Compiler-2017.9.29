#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/

/*struct F_frame_ {
    Temp_label name;
    F_accessList formals;
    int localNum;
};*/

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

const int F_wordSize = 4;

static F_access InFrame(int offset)
{
    F_access a = checked_malloc(sizeof(*a));
    a->kind = inFrame;
    a->u.offset = offset;
    return a;
}

static F_access InReg(Temp_temp t)
{
    F_access a = checked_malloc(sizeof(*a));
    a->kind = inReg;
    a->u.reg = t;
    return a;
}

static F_accessList F_AccessList2(int level, U_boolList formals);

F_accessList F_AccessList(F_access head, F_accessList tail)
{
	F_accessList al=checked_malloc(sizeof(*al));
    al->head = head;
    al->tail = tail;
    return al;
}

F_frame F_newFrame(Temp_label name, U_boolList formals)
{
	F_frame f=checked_malloc(sizeof(*f));
	f->name=name;
	f->formals=F_AccessList2(1,formals);
	f->localNum=0;
}

static F_accessList F_AccessList2(int level, U_boolList formals)
{
	/* assume every formal is escape */
	if(!formals)
		return NULL;
	return F_AccessList(InFrame(level*F_wordSize),F_AccessList2(++level,formals->tail));
}

Temp_label F_name(F_frame f)
{
	return f->name;
}

F_accessList F_formals(F_frame f)
{
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape)
{
	if(!escape)
		return InReg(Temp_newtemp());
	++f->localNum;
	return InFrame(-F_wordSize*f->localNum);
}

Temp_temp F_FP(void)
{
    return Temp_newtemp();
}

T_exp F_Exp(F_access acc, T_exp framePtr)
{
    if (acc->kind == inFrame)
        return T_Mem(T_Binop(T_plus, framePtr, T_Const(acc->u.offset)));
    return T_Temp(acc->u.reg);
}

T_exp F_externalCall(string s, T_expList args)
{
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}

F_frag F_StringFrag(Temp_label label, string str) {   
	F_frag f=checked_malloc(sizeof(*f));
	f->kind=F_stringFrag;
	f->u.stringg.label=label;
	f->u.stringg.str=str;
	return f;                                    
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	F_frag f=checked_malloc(sizeof(*f));
	f->kind=F_procFrag;
	f->u.proc.body=body;
	f->u.proc.frame=frame;
	return f;                                
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) { 
	F_fragList f=checked_malloc(sizeof(*f));
	f->head=head;
	f->tail=tail;
	return f;
}
