%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
	return NULL;
}

%}
  /* You can add lex definitions here. */

%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 
"\n" {adjust(); EM_newline(); continue;}
array {adjust();return ARRAY;}
if {adjust();return IF;}
then {adjust();return THEN;}
else {adjust();return ELSE;}
while {adjust();return WHILE;}
for {adjust();return FOR;}
to {adjust();return TO;}
do {adjust();return DO;}
let {adjust();return LET;}
in {adjust();return IN;}
end {adjust();return END;}
of {adjust();return OF;}
break {adjust();return BREAK;}
nil {adjust();return NIL;}
function {adjust();return FUNCTION;}
var {adjust();return VAR;}
type {adjust();return TYPE;}
[a-zA-Z][a-zA-Z0-9_]* {adjust();yylval.sval=String(yytext);return ID;}
\"[a-zA-Z0-9" ""\\t""\\n"]*\" {adjust();yylval.sval=String(yytext+1);yylval.sval[strlen(yylval.sval)-1]='\0';return STRING;}
[0-9]+ {adjust();yylval.ival=atoi(yytext);return INT;}
, {adjust();return COMMA;}
: {adjust();return COLON;}
; {adjust();return SEMICOLON;}
"(" {adjust();return LPAREN;}
")" {adjust();return RPAREN;}
"[" {adjust();return LBRACK;}
"]" {adjust();return RBRACK;}
"{" {adjust();return LBRACE;}
"}" {adjust();return RBRACE;}
"." {adjust();return DOT;}
"+" {adjust();return PLUS;}
- {adjust();return MINUS;}
"*" {adjust();return TIMES;}
"/" {adjust();return DIVIDE;}
= {adjust();return EQ;}
!= {adjust();return NEQ;}
"<" {adjust();return LT;}
"<=" {adjust();return LE;}
> {adjust();return GT;}
>= {adjust();return GE;}
& {adjust();return AND;}
"|" {adjust();return OR;}
:= {adjust();return ASSIGN;}
(" "|"\t")+ {adjust();continue;}
