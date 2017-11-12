#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

Ty_ty actual_ty(Ty_ty ty);

struct expty transVar(S_table venv,S_table tenv,A_var v)
{
	switch(v->kind)
	{
		case A_simpleVar:
		{
			E_enventry x=S_look(venv,v->u.simple);
			if(x&&x->kind==EvarEntry)
				return expTy(NULL,actual_ty(x->u.var.ty));
			EM_error(v->pos,"undefined variable %s",S_name(v->u.simple));
			return expTy(NULL,Ty_int());
		}
		case A_fieldVar:
		{
			struct expty left=transExp(venv,tenv,v->u.fieldvar.var);
			if(left->kind!=Ty_record)
			{
				EM_error(v->u.field.var->pos,"record type required");
				return expTy(Ty_Int());
			}
			Ty_fieldList field;
			for(field=left.ty->u.record;field!=NULL;field=field->tail)
			{
				if(field->head->name.sym==v->u.field.sym
					return expTy(NULL,field->head->ty);
			}
			EM_error(v->u.field.var->pos,"undefined field %s",S_name(v->u.field.sym));
			return expTy(NULL,Ty_Int());
		}
		case A_subscriptVar:
		{
			struct expty left=transVar(venv,tenv,v->u.subscript.var);
            if(var.ty->kind!=Ty_array)
            {
            	EM_error(v->u.subscript.var->pos, "array type required");
                return expTy(NULL,Ty_Int());
            }
            struct expty right = transExp(venv,tenv,v->u.subscript.exp);
            if(exp.ty->kind!=Ty_int)
            	EM_error(v->u.subscript.exp->pos, "integer required");
            else return expTy(NULL,var.ty->u.array);
        }
    }
}

struct expty transExp(S_table venv,S_table tenv,A_exp a)
{
	switch(a->kind)
	{
		case A_varExp:
			return transVar(venv,tenv,a->u.var);
		case A_nilExp:
			return expTy(NULL,Ty_Nil());
		case A_intExp:
			return expTy(NULL,Ty_Int());
		case A_stringExp:
			return expTy(NULL,Ty_String());
		case A_callExp:
		{
			E_enventry x=S_look(venv,a->u.call.func);
			if(!x||x->kind!=E_funEntry)
			{
				EM_error(a->pos,"undefined function %s", S_name(a->u.call.func));
				return expTy(NULL,Ty_Int());
			}
			A_expList args=a->u.call.args;
			Ty_tyList formals=x->u.fun.formals;
			while(args&&formals)
			{
				struct expty exp=transExp(venv, tenv, arg->head);
				if(formals->head->kind==Ty_array)
				{
					if(a!=b)
						EM_error(arg->head->pos,"para type mismatch");
				}
				else if(a->kind==Ty_record)
				{
					if(a!=b&&b->kind!=Ty_nil)
						EM_error(arg->head->pos,"para type mismatch");
				}
				else if(b->kind==Ty_record)
				{
					if(a!=b&&a->kind!=Ty_nil)
						EM_error(arg->head->pos,"para type mismatch");
				}
				else if(a->kind!=b->kind)
					EM_error(arg->head->pos,"para type mismatch");
				args=args->tail;
				formals=formals->tail;
			}
			if(args)
				EM_error(a->pos,"too many params in function %s", S_name(a->u.call.func));
			if(formals)
				EM_error(a->pos,"too less params in function %s", S_name(a->u.call.func));
			return expTy(NULL,x->u.fun.result);
		}
		case A_opExp:
		{
			struct expty left=transExp(venv,tenv,a->u.op.left);
			struct expty right=transExp(venv,tenv,a->u.op.right);
			if(left.ty->kind!=Ty_int)
				EM_error(a->u.op.left->pos,"integer required");
			if(right.ty->kind!=Ty_int)
				EM_error(a->u.op.right->pos,"integer required");
		}
		A_recordExp, 
	    A_seqExp, 
	    A_assignExp, 
	    A_ifExp,
	    A_whileExp, 
	    A_forExp, 
	    A_breakExp, 
	    A_letExp, 
	    A_arrayExp
}

Ty_ty actual_ty(Ty_ty ty)
{
	if(ty->kind==Ty_name)
    	return ty->u.name.ty;
    return ty;
}
