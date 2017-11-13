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

void SEM_transProg(A_exp exp)
{
	transExp(E_base_venv(), E_base_tenv(), exp);
}

struct expty transVar(S_table venv,S_table tenv,A_var v)
{
	switch(v->kind)
	{
		case A_simpleVar:
		{
			E_enventry x=S_look(venv,v->u.simple);
			if(x&&x->kind==E_varEntry)
				return expTy(NULL,actual_ty(x->u.var.ty));
			EM_error(v->pos,"undefined variable %s",S_name(v->u.simple));
			return expTy(NULL,Ty_Int());
		}
		case A_fieldVar:
		{
			struct expty left=transVar(venv,tenv,v->u.field.var);
			if(left.ty->kind!=Ty_record)
			{
				EM_error(v->u.field.var->pos,"not a record type");
				return expTy(NULL,Ty_Int());
			}
			Ty_fieldList field;
			for(field=left.ty->u.record;field!=NULL;field=field->tail)
			{
				if(field->head->name==v->u.field.sym)
					return expTy(NULL,field->head->ty);
			}
			EM_error(v->u.field.var->pos,"field %s doesn't exist",S_name(v->u.field.sym));
			return expTy(NULL,Ty_Int());
		}
		case A_subscriptVar:
		{
			struct expty left=transVar(venv,tenv,v->u.subscript.var);
            if(left.ty->kind!=Ty_array)
            {
            	EM_error(v->u.subscript.var->pos, "array type required");
                return expTy(NULL,Ty_Int());
            }
            struct expty right = transExp(venv,tenv,v->u.subscript.exp);
            if(right.ty->kind!=Ty_int)
            	EM_error(v->u.subscript.exp->pos, "integer required");
            else return expTy(NULL, left.ty->u.array);
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
			//fprintf(stderr,"[transExp] A_callExp\n");
			E_enventry x=S_look(venv,a->u.call.func);
			if(!x)
			{
				EM_error(a->pos,"undefined function %s", S_name(a->u.call.func));
				return expTy(NULL,Ty_Int());
			}
			A_expList args=a->u.call.args;
			Ty_tyList formals=x->u.fun.formals;
			while(args&&formals)
			{
				struct expty exp=transExp(venv,tenv,args->head);
				if(!TypeMatch(formals->head,exp.ty))
					EM_error(args->head->pos,"para type mismatch");
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
			//fprintf(stderr,"[transExp] A_opExp\n");
			A_oper oper = a->u.op.oper;
			struct expty left=transExp(venv,tenv,a->u.op.left);
			struct expty right=transExp(venv,tenv,a->u.op.right);
			if(oper==A_plusOp||oper==A_minusOp||oper==A_timesOp||oper==A_divideOp)
			{
				if(left.ty->kind!=Ty_int)
					EM_error(a->u.op.left->pos,"integer required");
				if(right.ty->kind!=Ty_int)
					EM_error(a->u.op.right->pos,"integer required");
			}
			/*else if(oper==A_eqOp||oper==A_neqOp)
			{
				if(!TypeMatch(left.ty,right.ty))
					EM_error(a->pos,"same type required");
			}*/
			else
			{
				//fprintf(stderr,"left kind %d,right kind %d",left.ty->kind,right.ty->kind);
				if((left.ty->kind!=Ty_int||right.ty->kind!=Ty_int)&&(left.ty->kind!=Ty_string||right.ty->kind!=Ty_string))
					EM_error(a->pos,"same type required");
			}
			return expTy(NULL,Ty_Int());
		}
		case A_recordExp:
		{
			//fprintf(stderr,"[transExp] A_recordExp\n");
			Ty_ty ty=actual_ty(S_look(tenv,a->u.record.typ));
			if(!ty)
			{
				EM_error(a->pos,"undefined type %s",S_name(a->u.record.typ));
			 	return expTy(NULL,Ty_Int());
			}
			if(ty->kind!=Ty_record)
			{
				EM_error(a->pos,"record type required");
			 	return expTy(NULL,Ty_Int());
			}
			//fprintf(stderr,"[transExp] ty type check finish\n");
			A_efieldList fields=a->u.record.fields;
			Ty_fieldList record=ty->u.record;
			struct expty exp;
			while(fields&&record)
			{
				exp=transExp(venv,tenv,fields->head->exp);
				if(fields->head->name!=record->head->name||!TypeMatch(record->head->ty,exp.ty))
			 		EM_error(fields->head->exp->pos,"type mismatch");
			 	fields=fields->tail;
			 	record=record->tail;
			}
			if(fields||record)
				EM_error(a->pos,"type mismatch");
			//fprintf(stderr,"[transExp] A_recordExp trans finish\n");
			return expTy(NULL,ty);
		}
	    case A_seqExp:
	    {
	    	//fprintf(stderr,"[transExp] A_seqExp\n");
	    	A_expList seq;
	    	struct expty ret;ret.ty=Ty_Void();
	    	for(seq=a->u.seq;seq;seq=seq->tail)
	    		ret=transExp(venv,tenv,seq->head);
	    	return ret;
	    }
	    case A_assignExp:
	    {
	    	//fprintf(stderr,"[transExp] A_assignExp\n");
	    	struct expty vty=transVar(venv,tenv,a->u.assign.var);
            struct expty ety=transExp(venv,tenv,a->u.assign.exp);
            //fprintf(stderr,"[transExp] transExp finish\n");
            //fprintf(stderr,"[transExp] S_look finish\n");
            if(vty.ty->kind==Ty_int&&a->u.assign.var->kind==A_simpleVar)
            {
            	E_enventry x=S_look(venv,a->u.assign.var->u.simple);
            	if(x->readonly)
            		EM_error(a->pos, "loop variable can't be assigned");
            }
			if(!TypeMatch(vty.ty,ety.ty))
				EM_error(a->pos,"unmatched assign exp");
            return expTy(NULL,Ty_Void());
        }
	    case A_ifExp:
	    {
	    	struct expty test=transExp(venv,tenv,a->u.iff.test);
            if(test.ty->kind!=Ty_int)
            	EM_error(a->u.iff.test->pos, "integer required");
            struct expty then = transExp(venv, tenv, a->u.iff.then);
			if(a->u.iff.elsee)
			{
				struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
                if (!TypeMatch(then.ty, elsee.ty))
                	EM_error(a->pos, "then exp and else exp type mismatch");
                return expTy(NULL,then.ty);
            }
            if(then.ty->kind!=Ty_void)
            	EM_error(a->pos, "if-then exp's body must produce no value");
            return expTy(NULL,Ty_Void());
        }
	    case A_whileExp:
	    {
	    	struct expty test = transExp(venv, tenv, a->u.whilee.test);
	    	if (test.ty->kind != Ty_int)
            	EM_error(a->u.whilee.test->pos, "integer required");
            struct expty body = transExp(venv, tenv, a->u.whilee.body);
            if (body.ty->kind != Ty_void)
            	EM_error(a->u.whilee.body->pos, "while body must produce no value");
            return expTy(NULL,Ty_Void());
        }
	    case A_forExp:
	    {
	    	struct expty lo = transExp(venv, tenv, a->u.forr.lo);
            struct expty hi = transExp(venv, tenv, a->u.forr.hi);
            if(lo.ty->kind!=Ty_int||hi.ty->kind!=Ty_int)
            	EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
            S_beginScope(venv);
            S_enter(venv,a->u.forr.var,E_ROVarEntry(Ty_Int()));
            struct expty body=transExp(venv, tenv, a->u.forr.body);
            if (body.ty->kind != Ty_void)
            	EM_error(a->u.forr.body->pos, "while body must produce no value");
            S_endScope(venv);
            return expTy(NULL,Ty_Void());
        }
	    case A_breakExp:
	    	return expTy(NULL,Ty_Void());
	    case A_letExp:
	    {
	    	//fprintf(stderr,"[transExp] let exp\n");
	    	A_decList decs;
	    	S_beginScope(venv);
            S_beginScope(tenv);
            for (decs = a->u.let.decs; decs; decs = decs->tail)
            	transDec(venv, tenv, decs->head);
            //fprintf(stderr,"[transExp] transDec finish\n");
            struct expty body = transExp(venv, tenv, a->u.let.body);
            S_endScope(tenv);
            S_endScope(venv);
            return body;
        }
	    case A_arrayExp:
	    {
	    	Ty_ty typ = actual_ty(S_look(tenv,a->u.array.typ));
	    	if (typ->kind != Ty_array)
	    	{
	    		EM_error(a->pos, "array type required");
                return expTy(NULL,Ty_Int());
            }
            struct expty size = transExp(venv, tenv, a->u.array.size);
            if (size.ty->kind != Ty_int)
            	EM_error(a->u.array.size->pos, "integer required");
            struct expty init = transExp(venv, tenv, a->u.array.init);
            if (!TypeMatch(typ->u.array, init.ty))
            	EM_error(a->u.array.init->pos, "type mismatch");
            return expTy(NULL,typ);
		}
	}
}
void transDec(S_table venv, S_table tenv, A_dec d)
{
	switch(d->kind)
	{
		case A_varDec:
		{
			//fprintf(stderr,"[transDec] A_varDec\n");
			struct expty init = transExp(venv, tenv, d->u.var.init);
			//fprintf(stderr,"[transDec] d->u.var.typ\n",S_name(d->u.var.typ));
            Ty_ty typ = actual_ty(S_look(tenv, d->u.var.typ));
           	if(typ)
           	{
                if (!TypeMatch(typ, init.ty))
                	EM_error(d->pos, "type mismatch");
                //fprintf(stderr,"[transDec] type match check finish\n");
                S_enter(venv, d->u.var.var, E_VarEntry(typ));
            } 
            else
            {
            	if (init.ty->kind == Ty_nil)
                	EM_error(d->pos, "init should not be nil without type specified");
                S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
            }
            //fprintf(stderr,"[transDec] A_varDec trans finish\n");
            break;
        }
        case A_functionDec:
        {
        	//fprintf(stderr,"[transDec] A_functionDec\n");
        	A_fundecList fun;
        	Ty_ty resultTy;
        	for (fun = d->u.function; fun; fun = fun->tail)
        	{
            	resultTy = S_look(tenv,fun->head->result);
                if(!resultTy)
                	resultTy = Ty_Void();
                Ty_tyList formalTy=FormalTyList(tenv,fun->head->params);
                A_fundecList funl;
                for(funl=fun->tail;funl;funl=funl->tail)
                {
                	if(funl->head->name==fun->head->name)
                	{
                		EM_error(d->pos, "two functions have the same name");
                		break;
                	}
                }
                //fprintf(stderr,"[transDec] %s\n",S_name(fun->head->name));
                S_enter(venv, fun->head->name, E_FunEntry(formalTy, resultTy));
			}
			Ty_tyList formals;
            E_enventry fe;
            A_fieldList params;
            struct expty exp;
			for (fun = d->u.function; fun; fun = fun->tail)
			{
				S_beginScope(venv);
                fe = S_look(venv, fun->head->name);
                formals = fe->u.fun.formals;
                resultTy = fe->u.fun.result;
                for(params=fun->head->params;params;params=params->tail,formals= formals->tail)
                	S_enter(venv, params->head->name, E_VarEntry(formals->head));
                exp = transExp(venv, tenv, fun->head->body);
                if (!TypeMatch(exp.ty, resultTy))
                {
                	if (resultTy->kind == Ty_void)
                    	EM_error(fun->head->body->pos, "procedure returns value");
                    else
                    	EM_error(fun->head->body->pos, "type mismatch");
                }
                S_endScope(venv);
            }
            break;
		}
		case A_typeDec:
		{
			//fprintf(stderr,"[transDec] A_typeDec\n");
			A_nametyList type;
			for (type = d->u.type; type; type = type->tail)
			{
				A_nametyList typel;
				for(typel=type->tail;typel;typel=typel->tail)
				{
					if(typel->head->name==type->head->name)
						EM_error(d->pos, "two types have the same name");
				}
				S_enter(tenv,type->head->name,Ty_Name(type->head->name, NULL));
			}
			int last=0;
			for (type = d->u.type; type; type = type->tail)
			{
				Ty_ty ty;
				switch(type->head->ty->kind)
				{
					case A_nameTy:
					{
						ty = S_look(tenv, type->head->name);
                        ty->u.name.sym = type->head->ty->u.name;
                        ++last;
                        break;
                    }
                    case A_recordTy:
                    {
                    	ty = S_look(tenv, type->head->name);
                        ty->kind = Ty_record;
                        ty->u.record = Ty_FieldList2(tenv,type->head->ty->u.record);
                        break;
                    }
                    case A_arrayTy:
                    {
                    	ty = S_look(tenv, type->head->name);
                        ty->kind = Ty_array;
                        ty->u.array = S_look(tenv, type->head->ty->u.array);
                        break;
                    }
                }
            }
            int tmp;
			while(last>0)
			{
				tmp=0;
				for (type = d->u.type; type; type = type->tail)
				{
					if (type->head->ty->kind == A_nameTy)
					{
						Ty_ty ty = S_look(tenv, type->head->name);
                        if (!ty->u.name.ty)
                        {
                        	Ty_ty ty2 = S_look(tenv, ty->u.name.sym);
                            if (ty2->kind == Ty_name)
                            {
                            	if (ty2->u.name.ty)
                                	ty->u.name.ty = ty2->u.name.ty;
                                else
                                	++tmp;
                            }
                            else
                            	ty->u.name.ty = ty2;
                        }
                    }
                }
                if(tmp==last)
                {
                	EM_error(d->pos, "illegal type cycle");
                	break;
                }
                last=tmp;
            }
            for (type = d->u.type; type; type = type->tail)
            {
            	switch(type->head->ty->kind)
				{
					case A_recordTy:
					{
						Ty_ty ty = S_look(tenv, type->head->name);
						if(!ty)//fprintf(stderr,"S_look fail");
                        ty->u.record = actual_tys(ty->u.record);
                        A_fieldList record=type->head->ty->u.record;
                        for(;record;record=record->tail)
                        {
                        	if(!S_look(tenv,record->head->typ))
                        	{
                        		EM_error(d->pos,"undefined type %s",S_name(record->head->typ));
                        		break;
                        	}
                        }
                        break;
                    }
                    case A_arrayTy:
                    {
                    	Ty_ty ty = S_look(tenv, type->head->name);
                        ty->u.array = actual_ty(ty->u.array);
                        break;
                    }
                }
            }
            break;
		}
	}
}

Ty_ty actual_ty(Ty_ty ty)
{
	if(!ty)
		return NULL;
	if(ty->kind==Ty_name)
    	return ty->u.name.ty;
    return ty;
}

bool TypeMatch(Ty_ty x,Ty_ty y)
{
	if(x->kind==Ty_array)
		return x==y;
	if(x->kind==Ty_record)
		return x==y||x->kind==Ty_nil;
	if(y->kind==Ty_record)
		return x==y||x->kind==Ty_nil;
	return x->kind==y->kind;
}

Ty_tyList FormalTyList(S_table tenv, A_fieldList fl)
{
	if(!fl)
		return NULL;
	return Ty_TyList(actual_ty(S_look(tenv,fl->head->typ)),FormalTyList(tenv,fl->tail));
}

Ty_fieldList Ty_FieldList2(S_table tenv, A_fieldList fl)
{
	if(!fl)
		return NULL;
	return Ty_FieldList(Ty_Field(fl->head->name,S_look(tenv,fl->head->typ)),Ty_FieldList2(tenv,fl->tail));
}

Ty_fieldList actual_tys(Ty_fieldList fl)
{
    if(!fl)
    	return NULL;
	return Ty_FieldList(Ty_Field(fl->head->name,actual_ty(fl->head->ty)),actual_tys(fl->tail));
    
}
