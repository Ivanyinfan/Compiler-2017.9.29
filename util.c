/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
void *checked_malloc(int len)
{void *p = malloc(len);
 if (!p) {
    fprintf(stderr,"\nRan out of memory!\n");
    exit(1);
 }
 return p;
}

string String(char *s)
{string p = checked_malloc(strlen(s)+1);
 strcpy(p,s);
 return p;
}

char *recoverString(char *s)
{
	int len=strlen(s);
	for(char *i=s;*i!='\0';++i)
	{
		if(*i=='\t'||*i=='\n')
			++len;
	}
	char *ret=checked_malloc(len);
	int j=0;
	for(char *i=s;*i!='\0';++i,++j)
	{
		if(*i=='\t')
		{
			ret[j]='\\';
			ret[++j]='t';
		}
		else if(*i=='\n')
		{
			ret[j]='\\';
			ret[++j]='n';
		}
		else
			ret[j]=*i;
	}
	return ret;
}

U_boolList U_BoolList(bool head, U_boolList tail)
{ U_boolList list = checked_malloc(sizeof(*list));
  list->head = head;
  list->tail = tail;
  return list;
}

int UBoolListLen(U_boolList l)
{
	int len=0;
	for(;l;l=l->tail)
		++len;
	return len;
}
