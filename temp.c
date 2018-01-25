/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

//struct Temp_temp_ {int num;bool spilled;};

int Temp_int(Temp_temp t)
{
	if(!t)
		return -1;
	return t->num;
}

string Temp_labelstring(Temp_label s)
{return S_name(s);
}

static int labels = 0;
static int ifTrueLabel=0;
static int ifFalseLabel=0;
static int whileTestLabel=0;
static int whileLoopLabel=0;
static int whileDoneLabel=0;

Temp_label Temp_newlabel(void)
{char buf[100];
 sprintf(buf,"L%d",labels++);
 //fprintf(stdout,"[temp][Temp_newlabel]%s\n",buf);fflush(stdout);
 return Temp_namedlabel(String(buf));
}

Temp_label Temp_IfTrueLabel(void)
{
	char buf[100];
 	sprintf(buf,"ifTrueL%d",ifTrueLabel++);
 	return Temp_namedlabel(String(buf));
}

Temp_label Temp_IfFalseLabel(void)
{
	char buf[100];
 	sprintf(buf,"ifFalseL%d",ifFalseLabel++);
 	return Temp_namedlabel(String(buf));
}

Temp_label Temp_WhileTestLabel(void)
{
	char buf[100];
 	sprintf(buf,"wTestL%d",whileTestLabel++);
 	return Temp_namedlabel(String(buf));
}

Temp_label Temp_WhileLoopLabel(void)
{
	char buf[100];
 	sprintf(buf,"wLoopL%d",whileLoopLabel++);
 	return Temp_namedlabel(String(buf));
}

Temp_label Temp_WhileDoneLabel(void)
{
	char buf[100];
 	sprintf(buf,"wDoneL%d",whileDoneLabel++);
 	return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
 p->num=temps++;
 {char r[16];
  sprintf(r, "%d", p->num);
  Temp_enter(Temp_name(), p, String(r));
 }
 return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under;};

//创建一个全局temp_map
Temp_map Temp_name(void) {
 static Temp_map m = NULL;
 if (!m) m=Temp_empty();
 return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) 
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

void printTempList(FILE *out,Temp_tempList l)
{
	for(;l;l=l->tail)
		fprintf(out,"%d ",l->head->num);
	fprintf(out,"\n");fflush(out);
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

void printTempLabelList(FILE *out,Temp_labelList l)
{
	for(;l;l=l->tail)
		fprintf(out,"%s ",Temp_labelstring(l->head));
	fprintf(out,"\n");fflush(out);
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

bool Temp_inTempList(Temp_temp t, Temp_tempList l)
{
    Temp_tempList p = l;
    for (; p; p = p->tail) {
        if (p->head == t) {
            return TRUE;
        }
    }
    return FALSE;
}

Temp_tempList Temp_SubTempList(Temp_tempList l, Temp_tempList r)
{
    Temp_tempList res = NULL;
    Temp_tempList p = l;
    for (; p; p = p->tail) {
        if (!Temp_inTempList(p->head, r)) {
            res = Temp_TempList(p->head, res);
        }
    }
    return res;
}

Temp_tempList Temp_UnionTempList(Temp_tempList l, Temp_tempList r)
{
    Temp_tempList res = r;
    Temp_tempList p = l;
    for (; p; p = p->tail) {
        if (!Temp_inTempList(p->head, r)) {
            res = Temp_TempList(p->head, res);
        }
    }
    return res;
}

bool Temp_TempListEqual(Temp_tempList l, Temp_tempList r)
{
    Temp_tempList p = l;
    for (; p; p = p->tail) {
        if (!Temp_inTempList(p->head, r)) {
            return FALSE;
        }
    }
    p = r;
    for (; p; p = p->tail) {
        if (!Temp_inTempList(p->head, l)) {
            return FALSE;
        }
    }
    return TRUE;
}

Temp_tempList Temp_replaceTempList(Temp_tempList l, Temp_temp old, Temp_temp new)
{
    if (l) {
        if (l->head == old) {
            return Temp_TempList(new, Temp_replaceTempList(l->tail, old, new));
        } else {
            return Temp_TempList(l->head, Temp_replaceTempList(l->tail, old, new));
        }
    } else {
        return NULL;
    }
}
