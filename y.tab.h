#define ID 257
#define STRING 258
#define INT 259
#define COMMA 260
#define COLON 261
#define SEMICOLON 262
#define LPAREN 263
#define RPAREN 264
#define LBRACK 265
#define RBRACK 266
#define LBRACE 267
#define RBRACE 268
#define DOT 269
#define PLUS 270
#define MINUS 271
#define TIMES 272
#define DIVIDE 273
#define EQ 274
#define NEQ 275
#define LT 276
#define LE 277
#define GT 278
#define GE 279
#define UMINUS 280
#define AND 281
#define OR 282
#define ASSIGN 283
#define ARRAY 284
#define IF 285
#define THEN 286
#define ELSE 287
#define WHILE 288
#define FOR 289
#define TO 290
#define DO 291
#define LET 292
#define IN 293
#define END 294
#define OF 295
#define BREAK 296
#define NIL 297
#define FUNCTION 298
#define VAR 299
#define TYPE 300
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
        A_expList explist;
        A_efieldList efieldlist;
        A_decList declist;
        A_dec dec;
        A_nametyList nametylist;
        A_namety namety;
        A_ty ty;
        A_fieldList fieldlist;
        A_field field;
        A_fundec fundec;
        A_fundecList fundeclist;
	/* et cetera */
	} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;
