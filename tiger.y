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

%left OR
%left AND
%nonassoc EQ NEQ GT LT GE LE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%%

program:	exp {absyn_root = $1;};

exp:   lvalue {$$=A_VarExp(EM_tokPos,$1);}
     | NIL {$$=A_NilExp(EM_tokPos);}
     | INT {$$=A_IntExp(EM_tokPos,$1);}
     | STRING {$$=A_StringExp(EM_tokPos,$1);}
     | ID LPAREN RPAREN {$$=A_CallExp(EM_tokPos,S_Symbol($1),NULL);}
     | ID LPAREN exp exps RPAREN {$$=A_CallExp(EM_tokPos,S_Symbol($1),A_ExpList($3,$4));}
     | exp OP exp {$$=A_OpExp(EM_tokPos,$2,$1,$3);}
     | ID LBRACE RBRACE {$$=A_RecordExp(EM_tokPos,S_Symbol($1),NULL);}
     | ID LBRACE efield efields RBRACE {$$=A_RecordExp(EM_tokPos,S_Symbol($1),A_EfieldList($3,$4));}
     | LPAREN exp exps LPAREN {$$=A_SeqExp(EM_tokPos,A_ExpList($2,$3));}
     | lvalue ASSIGN exp {$$=A_AssignExp(EM_tokPos,$1,$3);}
     | IF exp THEN exp ELSE exp {$$=A_IfExp(EM_tokPos,$2,$4,$6);}
     | IF exp THEN exp {$$=A_IfExp(EM_tokPos,$2,$4,NULL);}
     | WHILE exp DO exp {$$=A_WhileExp(EM_tokPos,$2,$4);}
     | FOR ID ASSIGN exp TO exp DO exp {$$=A_ForExp(EM_tokPos,S_Symbol($2),$4,$6,$8);}
     | BREAK {$$=A_BreakExp(EM_tokPos);}
     | LET decs IN exp END {$$=A_LetExp(EM_tokPos,$2,$4);}
     | ID LBRACK exp RBRACK OF exp {$$=A_ArrayExp(EM_tokPos,S_Symbol(ID),$3,$6);}
     ;

lvalue:   ID {$$=A_SimpleVar(EM_tokPos,S_Symbol($1);}
        | lvalue DOT ID {$$=A_FieldVar(EM_tokPos,$1,S_Symbol($3));}
        | lvalue LBRACK exp RBRACK {$$=A_SubscriptVar(EM_tokPos,$1,$3);}
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

tydec: TYPE ID EQ ty {$$=A_Namety(S_Symbol($2),$4);};

ty:   ID {$$=A_NameTy(EM_tokPos,S_Symbol($1);}
    | LBRACE tyfields RBRACE {$$=A_RecordTy(EM_tokPos,$2);}
    | ARRAY OF ID {$$=A_ArrayTy(EM_tokPos,S_Symbol($3);}
    ;

tyfields:   ID COLON ID tyfield {$$=A_FieldList(A_Field(EM_tokPos,S_Symbol($1),S_Symbol($3)),$4);}
          | {$$=NULL;}
          ;

tyfield:   COMMA ID COLON ID tyfield {$$=A_FieldList(A_Field(EM_tokPos,S_Symbol($2),S_Symbol($4)),$5);}
         | {$$=NULL;}
         ;

vardec     :   VAR ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol(""),$4);}
              |VAR ID COLON ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol($4),$6);}
              ; 

fundecs:   fundec fundecs {$$=A_FunctionDec(EM_tokPos,A_FundecList($1,$2);}
         | {$$=NULL;}
         ;

fundec:   FUNCTION ID LPAREN tyfields RPAREN EQ exp {$$=A_Fundec(EM_tokPos,S_Symbol($2),$4,NULL,$7);}
        | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp {$$=A_Fundec(EM_tokPos,S_Symbol($2),$4,S_Symbol($7),$9);}
        ;

exps:   SEMICOLON exp exps {$$=A_ExpList($2,$3);}
      | {$$=NULL;}
      ;

efield: ID EQ exp {$$=A_Efield(S_Symbol($1),$3);};

efields:   COMMA efield efields {$$=A_EfieldList($1,$2);}
         | {$$=NULL;}
         ;
