%{
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Xm/Xm.h>
#include "cal.h"

#ifdef linux
#define gettxt(a,b) b	/* for linux */
#endif

extern int	 parse_year;
int		 yacc_small;
int		 yacc_stringcolor;
char		*yacc_string;
int		 yacc_daycolor;
static int	 m, d, y;
%}

%union { int ival; char *sval; }
%type	<ival> color offset length expr pexpr number month reldate
%token <ival> NUMBER MONTH WDAY COLOR
%token <sval> STRING
%token	IN PLUS MINUS SMALL CYEAR LEAPYEAR
%token  LENGTH EASTER EQ_ NE LE GE LT GT PASCHA RESET

%left OR
%left AND
%right EQ_ NE LE GE LT GT
%left '-' '+'
%left '*' '/' '%'
%nonassoc '!'
%nonassoc UMINUS
%left '?' ':'

%start list

%%
list	:
	| RESET					{ reset_all_hdays(); }
	| list small color STRING color		{ yacc_stringcolor = $3;
						  yacc_string	= $4;
						  yacc_daycolor	= $5; }
	  entry					{ free(yacc_string); }
	;

small	:					{ yacc_small = 0; }
	| SMALL					{ yacc_small = 1; }
	;

color	:					{ $$ = 0; }
	| COLOR					{ $$ = $1; }
	;

entry	: EASTER offset length			{ seteaster($2, $3, 0); }
	| PASCHA offset length			{ seteaster($2, $3, 1); }
	| date offset length			{ setdate( m,  d,  y, $2, $3);}
	| WDAY offset length			{ setwday( 0, $1,  0, $2, $3);}
	| pexpr WDAY offset length		{ setwday($1, $2,  0, $3, $4);}
	| pexpr WDAY IN month offset length	{ setwday($1, $2, $4, $5, $6);}
	| WDAY pexpr date offset length		{ setdoff($1, $2,m,d,y,$4,$5);}
	;

offset	:					{ $$ =	0; }
	| PLUS expr				{ $$ =	$2; }
	| MINUS expr				{ $$ = -$2; }
	;

length	:					{ $$ =	1; }
	| LENGTH expr				{ $$ =	$2; }
	;

date	: pexpr '.' month			{ m = $3; d = $1; y = 0;  }
	| pexpr '.' month '.'			{ m = $3; d = $1; y = 0;  }
	| pexpr '.' month '.' expr		{ m = $3; d = $1; y = $5; }
	| month '/' pexpr			{ m = $1; d = $3; y = 0;  }
	| month '/' pexpr '/' pexpr		{ m = $1; d = $3; y = $5; }
	| MONTH pexpr				{ m = $1; d = $2; y = 0;  }
	| MONTH pexpr pexpr			{ m = $1; d = $2; y = $3; }
	| pexpr MONTH				{ m = $2; d = $1; y = 0;  }
	| pexpr MONTH pexpr			{ m = $2; d = $1; y = $3; }
	| pexpr '.' MONTH pexpr			{ m = $3; d = $1; y = $4; }
	| pexpr					{ monthday_from_day($1,
								&m, &d, &y); }
	;

reldate	: STRING				{ $$ = day_from_name($1); }
	| EASTER				{ $$ = day_from_easter(); }
	| pexpr '.' month			{ $$ = day_from_monthday
								($3, $1); }
	| pexpr '.' month '.'			{ $$ = day_from_monthday
								($3, $1); }
	| month '/' pexpr			{ $$ = day_from_monthday
								($1, $3); }
	| pexpr MONTH				{ $$ = day_from_monthday
								($2, $1); }
	| MONTH pexpr				{ $$ = day_from_monthday
								($1, $2); }
	| WDAY pexpr pexpr			{ $$ = day_from_wday($3, $1,
							$2 == -1 ? -1 : 0); }
	| pexpr WDAY IN month			{ int d=day_from_monthday
							($4 + ($1 == 999),1);
						  $$ = $1 == 999
						   ? day_from_wday(d,$2,-1)
						   : day_from_wday(d,$2,$1-1);}
	;

month	: MONTH | pexpr;

expr	: pexpr					{ $$ = $1; }
	| expr OR  expr				{ $$ = $1 || $3; }
	| expr AND expr				{ $$ = $1 && $3; }
	| expr EQ_ expr				{ $$ = $1 == $3; }
	| expr NE  expr				{ $$ = $1 != $3; }
	| expr LE  expr				{ $$ = $1 <= $3; }
	| expr GE  expr				{ $$ = $1 >= $3; }
	| expr LT  expr				{ $$ = $1 <  $3; }
	| expr GT  expr				{ $$ = $1 >  $3; }
	| expr '+' expr				{ $$ = $1 +  $3; }
	| expr '-' expr				{ $$ = $1 -  $3; }
	| expr '*' expr				{ $$ = $1 *  $3; }
	| expr '/' expr				{ $$ = $3 ?  $1 / $3 : 0; }
	| expr '%' expr				{ $$ = $3 ?  $1 % $3 : 0; }
	| expr '?' expr ':' expr		{ $$ = $1 ?  $3 : $5; }
	| '!' expr				{ $$ = !$2; }
	| '[' reldate ']'			{ $$ = $2; }
	;

pexpr	: '(' expr ')'				{ $$ = $2; }
	| number				{ $$ = $1; }
	;

number	: NUMBER
	| '-' NUMBER %prec UMINUS		{ $$ = -$2; }
	| CYEAR					{ $$ = parse_year; }
	| LEAPYEAR pexpr			{ $$ = !(($2) & 3); }
	;
%%
