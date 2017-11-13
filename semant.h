#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"

struct expty;

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void		 transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty		 transTy (              S_table tenv, A_ty a);

void SEM_transProg(A_exp exp);

/* Auxiliary FUnction */
Ty_ty actual_ty(Ty_ty);
bool TypeMatch(Ty_ty,Ty_ty);
Ty_tyList FormalTyList(S_table,A_fieldList);
Ty_fieldList Ty_FieldList2(S_table,A_fieldList);
Ty_fieldList actual_tys(Ty_fieldList);

#endif
