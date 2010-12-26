%option yylineno
%{
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include "minic_typedef.h"
#include "minic_parser.tab.h"

extern YYSTYPE yylval;
extern int evaluator(const char* ,int);

int sval_set = 0;
%}
Char [^\\ \n\t\v\b\r\f\a]|\\[ntvbrfa\\\'\"\?]
/* patten '\x**' not supported currently */
%%

[1-9][0-9]* {
    if(sval_set)
        free(yylval.tk.sval);
    yylval.tk.ival = evaluator(yytext, 10);
    sval_set = 0;
    return ICONSTANT;
}

0[0-7]* {
    if(sval_set)
        free(yylval.tk.sval);
    yylval.tk.ival = evaluator(yytext, 8);
    sval_set = 0;
    return ICONSTANT;
}

0x[0-9a-fA-F]+ {
	if(sval_set)
        free(yylval.tk.sval);
    yylval.tk.ival = evaluator(yytext, 16);
    sval_set = 0;
    return ICONSTANT;
}

\'{Char}\' {
	if(sval_set)
        free(yylval.tk.sval);
	yylval.tk.cval = yytext[0];
	sval_set = 0;
	return CCONSTANT;
}

\"{Char}*\" {
	if(sval_set)
        free(yylval.tk.sval);
	yylval.tk.sval = strdup(yytext);
	sval_set = 1;
	return SCONSTANT;
}

\/\*[^\*\/]*\*\/ {;}

"void"           { return VOID; }
"int"            { return INT; }
"char"           { return CHAR; }

"if"             { return IF; }
"else"           { return ELSE; }
"for"            { return FOR; }
"while"          { return WHILE; }
"return"         { return RETURN; }

"extern"         { return EXTERN; }
"register"       { return REGISTER; }

[\_a-zA-Z][\_0-9a-zA-Z]* {
    if(sval_set)
		free(yylval.tk.sval);
	yylval.tk.sval = strdup(yytext);
	sval_set = 1;
	return IDENT;
}

";"  { return SEMICOLON; }
"("  { return LEFTPARENTHESIS; }
")"  { return RIGHTPARENTHESIS; }
"{"  { return LEFTBRACE; }
"}"  { return RIGHTBRACE; }
"["  { return LEFTBRACKET; }
"]"  { return RIGHTBRACKET; }
","  { return COMMA; }
"="  { return ASSIGN; }
"+"  { return PLUS; }
"-"  { return MINUS; }
"*"  { return ASTERISK; }
">"  { return NLE; }
"<"  { return NGE; }
"!"  { return LNOT; }
"==" { return EQ; }
"++" { return PLUSPLUS; }
"--" { return MINUSMINUS; }
">=" { return GE; }
"<=" { return LE; }
"&&" { return LAND; }
"&"  { return AMPERSAND; }
"||" { return LOR; }
"!=" { return NEQ; }

[\ \n\t\r]+ {}
%%

int evaluator(const char* text, int radix)
{
	int len = strlen(text);
	int i, sum, temp;
	sum = 0;
	int lowerbound = 16 == radix ? 2 : 0;
	for(i = lowerbound; i < len; ++i)
	{
		switch(text[i])
		{
			case '0': /* fall through */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': temp = text[i] - '0'; break;
			case 'a': /* fall through */
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f': temp = text[i] - 'a' + 10; break;
			case 'A': /* fall through */
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F': temp = text[i] - 'A' + 10; break;
		}
		sum = sum * radix + temp;
	}
	return sum;
}
