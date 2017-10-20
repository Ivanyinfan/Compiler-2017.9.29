%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_exp exp;
	A_expList explist;
	A_var var;
	A_decList declist;
	A_dec  dec;
	A_efieldList efieldlist;
	A_efield  efield;
	A_namety namety;
	A_nametyList nametylist;
	A_fieldList fieldlist;
	A_field field;
	A_fundecList fundeclist;
	A_fundec fundec;
	A_ty ty;
	}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%type <exp> exp expseq
%type <explist> actuals  nonemptyactuals sequencing  sequencing_exps
%type <var>  lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec>  decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty 
%type <efield> rec_one
%type <nametylist> tydec
%type <namety>  tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%

program:	exp {absyn_root = $1;};

exp:   lvalue {$$=A_VarExp(EM_tokPos,$1);}
     | NIL {$$=A_NilExp(EM_tokPos);}
     | INT {$$=A_IntExp(EM_tokPos,$1);}
     | STRING {$$=A_StringExp(EM_tokPos,$1);}
     | ID LPAREN expList RPAREN {$$=A_CallExp(EM_tokPos,S_Symbol($1),);}
     | exp OP exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
     | {$$=A_RecordExp(EM_tokPos);}
     | {$$=A_SeqExp(EM_tokPos);}
     | lvalue ASSIGN exp {$$=A_AssignExp(EM_tokPos,$1,$3);}
     | IF exp THEN exp ELSE exp {$$=A_IfExp(EM_tokPos,$2,$4,$6);}
     | IF exp THEN exp {$$=A_IfExp(EM_tokPos,$2,$4,NULL);}
     | WHILE exp DO exp {$$=A_WhileExp(EM_tokPos,$2,$4);}
     | FOR ID ASSIGN exp TO exp DO exp {$$=A_ForExp(EM_tokPos,S_symbol($2),$4,$6,$8);}
     | BREAK {$$=A_BreakExp(EM_tokPos);}
     | LET decs IN exp END {$$=A_LetExp(EM_tokPos,$2,$4);}
     | ID LBRACK exp RBRACK OF exp {$$=A_ArrayExp(EM_tokPos,S_symbol(ID),$3,$6);}
     ;

decs:   dec decs {$$=A_decList($1,$2);}
      | {$$=NULL;}
      ;

dec:   tydecs {$$=A_TypeDec(EM_tokPos,$1);}
     | vardec {$$=$1;}
     | fundec {$$=$1;}
     ;

tydecs:   tydec tydecs {$$=A_NametyList($1,$2);}
        | {$$=NULL;}
        ;

tydec: TYPE ID EQ ty {$$=A_Namety(S_symbol($2),$4);};

ty:   ID {$$=A_NameTy(EM_tokPos,S_symbol($1);}
    | LBRACE tyfields RBRACE {$$=A_RecordTy(EM_tokPos,$2);}
    | ARRAY OF ID {$$=A_ArrayTy(EM_tokPos,S_symbol($3);}
    ;

tyfields:   tyfield tyfields {$$=A_NameList($1,$2);}
          | COMMA {$$=NULL;}
          | {$$=NULL;}
          ;

vardec     :   VAR ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol(""),$4);}
              |VAR ID COLON ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol($4),$6);}
              ; 

fundec:   FUNCTION ID LPAREN tyfields RPAREN EQ exp {$$=A_FunctionDec(EM_tokPos,
