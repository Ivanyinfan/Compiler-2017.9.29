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

struct F_frame_ {
    Temp_label name;
    F_accessList formals;
    int localNum;
};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

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
	f->formals=F_AccessList2(formals);
	localNum=0;
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
	return InFrame(-F_worldSize*f->localNum);
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
