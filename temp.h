/*
 * temp.h 
 *
 */

#ifndef TEMP_H
#define TEMP_H

struct Temp_temp_ {int num;bool spilled;};
typedef struct Temp_temp_ *Temp_temp;
Temp_temp Temp_newtemp(void);
int Temp_int(Temp_temp);

typedef struct Temp_tempList_ *Temp_tempList;
struct Temp_tempList_ { Temp_temp head; Temp_tempList tail;};
Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t);
void printTempList(FILE *out,Temp_tempList l);

typedef S_symbol Temp_label;
Temp_label Temp_newlabel(void);
Temp_label Temp_namedlabel(string name);
string Temp_labelstring(Temp_label s);
Temp_label Temp_IfTrueLabel(void);
Temp_label Temp_IfFalseLabel(void);
Temp_label Temp_WhileTestLabel(void);
Temp_label Temp_WhileLoopLabel(void);
Temp_label Temp_WhileDoneLabel(void);

typedef struct Temp_labelList_ *Temp_labelList;
struct Temp_labelList_ { Temp_label head; Temp_labelList tail;};
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t);
void printTempLabelList(FILE *out,Temp_labelList l);

typedef struct Temp_map_ *Temp_map;
Temp_map Temp_empty(void);
Temp_map Temp_layerMap(Temp_map over, Temp_map under);
void Temp_enter(Temp_map m, Temp_temp t, string s);
string Temp_look(Temp_map m, Temp_temp t);
void Temp_dumpMap(FILE *out, Temp_map m);

Temp_map Temp_name(void);

bool Temp_inTempList(Temp_temp t, Temp_tempList l);
Temp_tempList Temp_SubTempList(Temp_tempList l, Temp_tempList r);
Temp_tempList Temp_UnionTempList(Temp_tempList l, Temp_tempList r);
bool Temp_TempListEqual(Temp_tempList l, Temp_tempList r);
Temp_tempList Temp_replaceTempList(Temp_tempList l, Temp_temp old, Temp_temp new);
#endif
