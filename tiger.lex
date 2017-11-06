%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "errormsg.h"
#include "absyn.h"
#include "y.tab.h"

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
int comment_layer=0;
/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
	int len=strlen(str);
	string ret=checked_malloc(len-1);
	int i,j=0;
	for(i=0;i<len;++i)
	{
		if(i==0||i==len-1)
			continue;
		if(str[i]=='\\')
		{
			if(str[i+1]=='t')
			{
				ret[j]='\t';
				++i;
			}
			else if(str[i+1]=='n')
			{
				ret[j]='\n';
				++i;
			}
			else
				ret[j]=='\\';
		}
		else
			ret[j]=str[i];
		++j;
	}
	if(j==0)
		ret="";
	else
		ret[j]='\0';		
	return ret;
}

%}
  /* You can add lex definitions here. */
%Start COMMENT
%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 
<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>array {adjust();return ARRAY;}
<INITIAL>if {adjust();return IF;}
<INITIAL>then {adjust();return THEN;}
<INITIAL>else {adjust();return ELSE;}
<INITIAL>while {adjust();return WHILE;}
<INITIAL>for {adjust();return FOR;}
<INITIAL>to {adjust();return TO;}
<INITIAL>do {adjust();return DO;}
<INITIAL>let {adjust();return LET;}
<INITIAL>in {adjust();return IN;}
<INITIAL>end {adjust();return END;}
<INITIAL>of {adjust();return OF;}
<INITIAL>break {adjust();return BREAK;}
<INITIAL>nil {adjust();return NIL;}
<INITIAL>function {adjust();return FUNCTION;}
<INITIAL>var {adjust();return VAR;}
<INITIAL>type {adjust();return TYPE;}
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust();yylval.sval=String(yytext);return ID;}
<INITIAL>\"[a-zA-Z0-9\- \.\\]*\" {adjust();yylval.sval=getstr(yytext);return STRING;}
<INITIAL>[0-9]+ {adjust();yylval.ival=atoi(yytext);return INT;}
<INITIAL>, {adjust();return COMMA;}
<INITIAL>: {adjust();return COLON;}
<INITIAL>; {adjust();return SEMICOLON;}
<INITIAL>"(" {adjust();return LPAREN;}
<INITIAL>")" {adjust();return RPAREN;}
<INITIAL>"[" {adjust();return LBRACK;}
<INITIAL>"]" {adjust();return RBRACK;}
<INITIAL>"{" {adjust();return LBRACE;}
<INITIAL>"}" {adjust();return RBRACE;}
<INITIAL>"." {adjust();return DOT;}
<INITIAL>"+" {adjust();return PLUS;}
<INITIAL>- {adjust();return MINUS;}
<INITIAL>"*" {adjust();return TIMES;}
<INITIAL>"/" {adjust();return DIVIDE;}
<INITIAL>= {adjust();return EQ;}
<INITIAL>!=|<> {adjust();return NEQ;}
<INITIAL>"<" {adjust();return LT;}
<INITIAL>"<=" {adjust();return LE;}
<INITIAL>> {adjust();return GT;}
<INITIAL>>= {adjust();return GE;}
<INITIAL>& {adjust();return AND;}
<INITIAL>"|" {adjust();return OR;}
<INITIAL>:= {adjust();return ASSIGN;}
<INITIAL>(" "|"\t")+ {adjust();continue;}
<INITIAL>\/\* {adjust();comment_layer++;BEGIN COMMENT;}
<INITIAL>. {adjust();EM_error(charPos,"illegal character");}
<COMMENT>\/\* {adjust();comment_layer++;}
<COMMENT>[a-zA-Z0-9\-\.\"\(\);\\:=,!\n@#$% ]* {adjust();}
<COMMENT>\*\/ {adjust();comment_layer--;if(comment_layer==0)BEGIN INITIAL;}
<COMMENT>\*|\/ {adjust();}
