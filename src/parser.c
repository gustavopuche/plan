/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "parser.y"

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

#line 90 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NUMBER = 258,                  /* NUMBER  */
    MONTH = 259,                   /* MONTH  */
    WDAY = 260,                    /* WDAY  */
    COLOR = 261,                   /* COLOR  */
    STRING = 262,                  /* STRING  */
    IN = 263,                      /* IN  */
    PLUS = 264,                    /* PLUS  */
    MINUS = 265,                   /* MINUS  */
    SMALL = 266,                   /* SMALL  */
    CYEAR = 267,                   /* CYEAR  */
    LEAPYEAR = 268,                /* LEAPYEAR  */
    LENGTH = 269,                  /* LENGTH  */
    EASTER = 270,                  /* EASTER  */
    EQ_ = 271,                     /* EQ_  */
    NE = 272,                      /* NE  */
    LE = 273,                      /* LE  */
    GE = 274,                      /* GE  */
    LT = 275,                      /* LT  */
    GT = 276,                      /* GT  */
    PASCHA = 277,                  /* PASCHA  */
    RESET = 278,                   /* RESET  */
    OR = 279,                      /* OR  */
    AND = 280,                     /* AND  */
    UMINUS = 281                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define NUMBER 258
#define MONTH 259
#define WDAY 260
#define COLOR 261
#define STRING 262
#define IN 263
#define PLUS 264
#define MINUS 265
#define SMALL 266
#define CYEAR 267
#define LEAPYEAR 268
#define LENGTH 269
#define EASTER 270
#define EQ_ 271
#define NE 272
#define LE 273
#define GE 274
#define LT 275
#define GT 276
#define PASCHA 277
#define RESET 278
#define OR 279
#define AND 280
#define UMINUS 281

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 20 "parser.y"
 int ival; char *sval; 

#line 198 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NUMBER = 3,                     /* NUMBER  */
  YYSYMBOL_MONTH = 4,                      /* MONTH  */
  YYSYMBOL_WDAY = 5,                       /* WDAY  */
  YYSYMBOL_COLOR = 6,                      /* COLOR  */
  YYSYMBOL_STRING = 7,                     /* STRING  */
  YYSYMBOL_IN = 8,                         /* IN  */
  YYSYMBOL_PLUS = 9,                       /* PLUS  */
  YYSYMBOL_MINUS = 10,                     /* MINUS  */
  YYSYMBOL_SMALL = 11,                     /* SMALL  */
  YYSYMBOL_CYEAR = 12,                     /* CYEAR  */
  YYSYMBOL_LEAPYEAR = 13,                  /* LEAPYEAR  */
  YYSYMBOL_LENGTH = 14,                    /* LENGTH  */
  YYSYMBOL_EASTER = 15,                    /* EASTER  */
  YYSYMBOL_EQ_ = 16,                       /* EQ_  */
  YYSYMBOL_NE = 17,                        /* NE  */
  YYSYMBOL_LE = 18,                        /* LE  */
  YYSYMBOL_GE = 19,                        /* GE  */
  YYSYMBOL_LT = 20,                        /* LT  */
  YYSYMBOL_GT = 21,                        /* GT  */
  YYSYMBOL_PASCHA = 22,                    /* PASCHA  */
  YYSYMBOL_RESET = 23,                     /* RESET  */
  YYSYMBOL_OR = 24,                        /* OR  */
  YYSYMBOL_AND = 25,                       /* AND  */
  YYSYMBOL_26_ = 26,                       /* '-'  */
  YYSYMBOL_27_ = 27,                       /* '+'  */
  YYSYMBOL_28_ = 28,                       /* '*'  */
  YYSYMBOL_29_ = 29,                       /* '/'  */
  YYSYMBOL_30_ = 30,                       /* '%'  */
  YYSYMBOL_31_ = 31,                       /* '!'  */
  YYSYMBOL_UMINUS = 32,                    /* UMINUS  */
  YYSYMBOL_33_ = 33,                       /* '?'  */
  YYSYMBOL_34_ = 34,                       /* ':'  */
  YYSYMBOL_35_ = 35,                       /* '.'  */
  YYSYMBOL_36_ = 36,                       /* '['  */
  YYSYMBOL_37_ = 37,                       /* ']'  */
  YYSYMBOL_38_ = 38,                       /* '('  */
  YYSYMBOL_39_ = 39,                       /* ')'  */
  YYSYMBOL_YYACCEPT = 40,                  /* $accept  */
  YYSYMBOL_list = 41,                      /* list  */
  YYSYMBOL_42_1 = 42,                      /* $@1  */
  YYSYMBOL_small = 43,                     /* small  */
  YYSYMBOL_color = 44,                     /* color  */
  YYSYMBOL_entry = 45,                     /* entry  */
  YYSYMBOL_offset = 46,                    /* offset  */
  YYSYMBOL_length = 47,                    /* length  */
  YYSYMBOL_date = 48,                      /* date  */
  YYSYMBOL_reldate = 49,                   /* reldate  */
  YYSYMBOL_month = 50,                     /* month  */
  YYSYMBOL_expr = 51,                      /* expr  */
  YYSYMBOL_pexpr = 52,                     /* pexpr  */
  YYSYMBOL_number = 53                     /* number  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   283

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  40
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  14
/* YYNRULES -- Number of rules.  */
#define YYNRULES  66
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  125

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   281


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    31,     2,     2,     2,    30,     2,     2,
      38,    39,    28,    27,     2,    26,    35,    29,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    34,     2,
       2,     2,     2,    33,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    36,     2,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    32
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    39,    39,    40,    41,    41,    47,    48,    51,    52,
      55,    56,    57,    58,    59,    60,    61,    64,    65,    66,
      69,    70,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    87,    88,    89,    91,    93,    95,    97,
      99,   101,   108,   108,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   129,   130,   133,   134,   135,   136
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "NUMBER", "MONTH",
  "WDAY", "COLOR", "STRING", "IN", "PLUS", "MINUS", "SMALL", "CYEAR",
  "LEAPYEAR", "LENGTH", "EASTER", "EQ_", "NE", "LE", "GE", "LT", "GT",
  "PASCHA", "RESET", "OR", "AND", "'-'", "'+'", "'*'", "'/'", "'%'", "'!'",
  "UMINUS", "'?'", "':'", "'.'", "'['", "']'", "'('", "')'", "$accept",
  "list", "$@1", "small", "color", "entry", "offset", "length", "date",
  "reldate", "month", "expr", "pexpr", "number", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-22)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-44)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -19,   -22,     1,   -22,   -22,     8,   -22,     9,     8,   -22,
      33,   -22,   139,   124,   -22,   139,    -3,    -3,    15,    48,
     -22,    -3,    -9,    18,   -22,   139,    48,    48,    10,   135,
     -22,    10,    10,   -22,    48,   117,   171,   -22,    10,   139,
     139,    57,   141,   -22,   214,   214,    48,   -22,    -3,    27,
     -22,   -22,     2,   139,   139,   -22,   -22,     4,     5,    22,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    48,   -22,   -22,    13,   -22,   156,    10,
     139,    14,   -22,   214,    10,   -22,   139,   -22,   139,   -22,
      31,   156,   250,   250,   250,   250,   250,   250,   232,   250,
      60,    60,     2,     2,     2,   195,   139,   -22,    -3,   -22,
     -22,    48,   -22,   -22,   -22,   156,    17,    48,   -22,    10,
     214,   -22,   -22,   -22,   -22
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     3,     6,     1,     7,     8,     9,     0,     8,     4,
       0,    63,    42,    17,    65,     0,    17,    17,     0,     0,
       5,    17,     0,    32,    62,    27,     0,     0,    20,     0,
      66,    20,    20,    64,     0,     0,     0,    44,    20,     0,
      29,    17,     0,    28,    18,    19,     0,    13,    17,    32,
      10,    11,    59,    42,     0,    33,    34,     0,     0,    43,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    61,    12,    25,    30,     0,    20,
      42,    22,    43,    21,    20,    39,     0,    60,     0,    38,
       0,     0,    47,    48,    49,    50,    51,    52,    45,    46,
      54,    53,    55,    56,    57,     0,     0,    42,    17,    14,
      31,    23,    16,    40,    37,     0,    35,     0,    26,    20,
      24,    41,    36,    58,    15
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -22,   -22,   -22,   -22,    42,   -22,    -8,   -21,    25,   -22,
     -14,    46,   -10,   -22
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     2,    10,     5,     7,    20,    28,    47,    21,    57,
      22,    36,    37,    24
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      23,     3,    25,    29,     1,    30,    26,    27,    31,    32,
      50,    51,     4,    38,     6,    43,     8,    75,    33,    49,
      39,    58,    40,    41,    46,    59,    89,    90,    81,    76,
      77,    40,    82,    79,    88,    73,    11,    12,    13,   115,
      84,    87,   106,    85,    86,    14,    15,   -43,    16,   111,
       9,    11,   122,    42,    48,    17,   -43,    91,   109,    18,
      14,    15,    42,   112,   108,    78,    26,    27,    82,     0,
     110,    19,    44,    45,    18,     0,   113,   116,   114,    34,
      52,    82,     0,     0,    35,     0,    19,     0,    70,    71,
      72,     0,    83,    73,     0,     0,   118,     0,   124,     0,
     119,   121,     0,     0,     0,    82,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      11,    53,    54,     0,    55,     0,     0,    11,     0,    14,
      15,     0,    56,    26,    27,     0,    14,    15,    11,    12,
       0,     0,    11,    18,    11,    80,     0,    14,    15,     0,
      18,    14,    15,    14,    15,    19,     0,   120,     0,    11,
     107,    18,    19,   123,     0,    18,     0,    18,    14,    15,
       0,     0,     0,    19,     0,     0,     0,    19,     0,    19,
       0,     0,    18,     0,     0,     0,     0,    60,    61,    62,
      63,    64,    65,     0,    19,    66,    67,    68,    69,    70,
      71,    72,     0,     0,    73,     0,     0,     0,     0,     0,
      74,    60,    61,    62,    63,    64,    65,     0,     0,    66,
      67,    68,    69,    70,    71,    72,     0,     0,    73,   117,
      60,    61,    62,    63,    64,    65,     0,     0,    66,    67,
      68,    69,    70,    71,    72,     0,     0,    73,    60,    61,
      62,    63,    64,    65,     0,     0,     0,    67,    68,    69,
      70,    71,    72,     0,     0,    73,    60,    61,    62,    63,
      64,    65,     0,     0,     0,     0,    68,    69,    70,    71,
      72,     0,     0,    73
};

static const yytype_int8 yycheck[] =
{
      10,     0,    12,    13,    23,    15,     9,    10,    16,    17,
      31,    32,    11,    21,     6,    25,     7,    38,     3,    29,
      29,    35,     4,     5,    14,    35,     4,     5,    42,    39,
      40,     4,    42,    41,    29,    33,     3,     4,     5,     8,
      48,    37,    29,    53,    54,    12,    13,    29,    15,    35,
       8,     3,    35,    35,    29,    22,    29,    35,    79,    26,
      12,    13,    35,    84,    78,     8,     9,    10,    78,    -1,
      80,    38,    26,    27,    26,    -1,    86,    91,    88,    31,
      34,    91,    -1,    -1,    36,    -1,    38,    -1,    28,    29,
      30,    -1,    46,    33,    -1,    -1,   106,    -1,   119,    -1,
     108,   115,    -1,    -1,    -1,   115,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
       3,     4,     5,    -1,     7,    -1,    -1,     3,    -1,    12,
      13,    -1,    15,     9,    10,    -1,    12,    13,     3,     4,
      -1,    -1,     3,    26,     3,     4,    -1,    12,    13,    -1,
      26,    12,    13,    12,    13,    38,    -1,   111,    -1,     3,
       4,    26,    38,   117,    -1,    26,    -1,    26,    12,    13,
      -1,    -1,    -1,    38,    -1,    -1,    -1,    38,    -1,    38,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    16,    17,    18,
      19,    20,    21,    -1,    38,    24,    25,    26,    27,    28,
      29,    30,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,
      39,    16,    17,    18,    19,    20,    21,    -1,    -1,    24,
      25,    26,    27,    28,    29,    30,    -1,    -1,    33,    34,
      16,    17,    18,    19,    20,    21,    -1,    -1,    24,    25,
      26,    27,    28,    29,    30,    -1,    -1,    33,    16,    17,
      18,    19,    20,    21,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    -1,    -1,    33,    16,    17,    18,    19,
      20,    21,    -1,    -1,    -1,    -1,    26,    27,    28,    29,
      30,    -1,    -1,    33
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    23,    41,     0,    11,    43,     6,    44,     7,    44,
      42,     3,     4,     5,    12,    13,    15,    22,    26,    38,
      45,    48,    50,    52,    53,    52,     9,    10,    46,    52,
      52,    46,    46,     3,    31,    36,    51,    52,    46,    29,
       4,     5,    35,    52,    51,    51,    14,    47,    48,    52,
      47,    47,    51,     4,     5,     7,    15,    49,    50,    52,
      16,    17,    18,    19,    20,    21,    24,    25,    26,    27,
      28,    29,    30,    33,    39,    47,    52,    52,     8,    46,
       4,    50,    52,    51,    46,    52,    52,    37,    29,     4,
       5,    35,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    29,     4,    50,    47,
      52,    35,    47,    52,    52,     8,    50,    34,    52,    46,
      51,    50,    35,    51,    47
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    40,    41,    41,    42,    41,    43,    43,    44,    44,
      45,    45,    45,    45,    45,    45,    45,    46,    46,    46,
      47,    47,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    50,    50,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    52,    52,    53,    53,    53,    53
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     1,     0,     7,     0,     1,     0,     1,
       3,     3,     3,     3,     4,     6,     5,     0,     2,     2,
       0,     2,     3,     4,     5,     3,     5,     2,     3,     2,
       3,     4,     1,     1,     1,     3,     4,     3,     2,     2,
       3,     4,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     5,     2,
       3,     3,     1,     1,     2,     1,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 3: /* list: RESET  */
#line 40 "parser.y"
                                                { reset_all_hdays(); }
#line 1336 "y.tab.c"
    break;

  case 4: /* $@1: %empty  */
#line 41 "parser.y"
                                                { yacc_stringcolor = (yyvsp[-2].ival);
						  yacc_string	= (yyvsp[-1].sval);
						  yacc_daycolor	= (yyvsp[0].ival); }
#line 1344 "y.tab.c"
    break;

  case 5: /* list: list small color STRING color $@1 entry  */
#line 44 "parser.y"
                                                { free(yacc_string); }
#line 1350 "y.tab.c"
    break;

  case 6: /* small: %empty  */
#line 47 "parser.y"
                                                { yacc_small = 0; }
#line 1356 "y.tab.c"
    break;

  case 7: /* small: SMALL  */
#line 48 "parser.y"
                                                { yacc_small = 1; }
#line 1362 "y.tab.c"
    break;

  case 8: /* color: %empty  */
#line 51 "parser.y"
                                                { (yyval.ival) = 0; }
#line 1368 "y.tab.c"
    break;

  case 9: /* color: COLOR  */
#line 52 "parser.y"
                                                { (yyval.ival) = (yyvsp[0].ival); }
#line 1374 "y.tab.c"
    break;

  case 10: /* entry: EASTER offset length  */
#line 55 "parser.y"
                                                { seteaster((yyvsp[-1].ival), (yyvsp[0].ival), 0); }
#line 1380 "y.tab.c"
    break;

  case 11: /* entry: PASCHA offset length  */
#line 56 "parser.y"
                                                { seteaster((yyvsp[-1].ival), (yyvsp[0].ival), 1); }
#line 1386 "y.tab.c"
    break;

  case 12: /* entry: date offset length  */
#line 57 "parser.y"
                                                { setdate( m,  d,  y, (yyvsp[-1].ival), (yyvsp[0].ival));}
#line 1392 "y.tab.c"
    break;

  case 13: /* entry: WDAY offset length  */
#line 58 "parser.y"
                                                { setwday( 0, (yyvsp[-2].ival),  0, (yyvsp[-1].ival), (yyvsp[0].ival));}
#line 1398 "y.tab.c"
    break;

  case 14: /* entry: pexpr WDAY offset length  */
#line 59 "parser.y"
                                                { setwday((yyvsp[-3].ival), (yyvsp[-2].ival),  0, (yyvsp[-1].ival), (yyvsp[0].ival));}
#line 1404 "y.tab.c"
    break;

  case 15: /* entry: pexpr WDAY IN month offset length  */
#line 60 "parser.y"
                                                { setwday((yyvsp[-5].ival), (yyvsp[-4].ival), (yyvsp[-2].ival), (yyvsp[-1].ival), (yyvsp[0].ival));}
#line 1410 "y.tab.c"
    break;

  case 16: /* entry: WDAY pexpr date offset length  */
#line 61 "parser.y"
                                                { setdoff((yyvsp[-4].ival), (yyvsp[-3].ival),m,d,y,(yyvsp[-1].ival),(yyvsp[0].ival));}
#line 1416 "y.tab.c"
    break;

  case 17: /* offset: %empty  */
#line 64 "parser.y"
                                                { (yyval.ival) =	0; }
#line 1422 "y.tab.c"
    break;

  case 18: /* offset: PLUS expr  */
#line 65 "parser.y"
                                                { (yyval.ival) =	(yyvsp[0].ival); }
#line 1428 "y.tab.c"
    break;

  case 19: /* offset: MINUS expr  */
#line 66 "parser.y"
                                                { (yyval.ival) = -(yyvsp[0].ival); }
#line 1434 "y.tab.c"
    break;

  case 20: /* length: %empty  */
#line 69 "parser.y"
                                                { (yyval.ival) =	1; }
#line 1440 "y.tab.c"
    break;

  case 21: /* length: LENGTH expr  */
#line 70 "parser.y"
                                                { (yyval.ival) =	(yyvsp[0].ival); }
#line 1446 "y.tab.c"
    break;

  case 22: /* date: pexpr '.' month  */
#line 73 "parser.y"
                                                { m = (yyvsp[0].ival); d = (yyvsp[-2].ival); y = 0;  }
#line 1452 "y.tab.c"
    break;

  case 23: /* date: pexpr '.' month '.'  */
#line 74 "parser.y"
                                                { m = (yyvsp[-1].ival); d = (yyvsp[-3].ival); y = 0;  }
#line 1458 "y.tab.c"
    break;

  case 24: /* date: pexpr '.' month '.' expr  */
#line 75 "parser.y"
                                                { m = (yyvsp[-2].ival); d = (yyvsp[-4].ival); y = (yyvsp[0].ival); }
#line 1464 "y.tab.c"
    break;

  case 25: /* date: month '/' pexpr  */
#line 76 "parser.y"
                                                { m = (yyvsp[-2].ival); d = (yyvsp[0].ival); y = 0;  }
#line 1470 "y.tab.c"
    break;

  case 26: /* date: month '/' pexpr '/' pexpr  */
#line 77 "parser.y"
                                                { m = (yyvsp[-4].ival); d = (yyvsp[-2].ival); y = (yyvsp[0].ival); }
#line 1476 "y.tab.c"
    break;

  case 27: /* date: MONTH pexpr  */
#line 78 "parser.y"
                                                { m = (yyvsp[-1].ival); d = (yyvsp[0].ival); y = 0;  }
#line 1482 "y.tab.c"
    break;

  case 28: /* date: MONTH pexpr pexpr  */
#line 79 "parser.y"
                                                { m = (yyvsp[-2].ival); d = (yyvsp[-1].ival); y = (yyvsp[0].ival); }
#line 1488 "y.tab.c"
    break;

  case 29: /* date: pexpr MONTH  */
#line 80 "parser.y"
                                                { m = (yyvsp[0].ival); d = (yyvsp[-1].ival); y = 0;  }
#line 1494 "y.tab.c"
    break;

  case 30: /* date: pexpr MONTH pexpr  */
#line 81 "parser.y"
                                                { m = (yyvsp[-1].ival); d = (yyvsp[-2].ival); y = (yyvsp[0].ival); }
#line 1500 "y.tab.c"
    break;

  case 31: /* date: pexpr '.' MONTH pexpr  */
#line 82 "parser.y"
                                                { m = (yyvsp[-1].ival); d = (yyvsp[-3].ival); y = (yyvsp[0].ival); }
#line 1506 "y.tab.c"
    break;

  case 32: /* date: pexpr  */
#line 83 "parser.y"
                                                { monthday_from_day((yyvsp[0].ival),
								&m, &d, &y); }
#line 1513 "y.tab.c"
    break;

  case 33: /* reldate: STRING  */
#line 87 "parser.y"
                                                { (yyval.ival) = day_from_name((yyvsp[0].sval)); }
#line 1519 "y.tab.c"
    break;

  case 34: /* reldate: EASTER  */
#line 88 "parser.y"
                                                { (yyval.ival) = day_from_easter(); }
#line 1525 "y.tab.c"
    break;

  case 35: /* reldate: pexpr '.' month  */
#line 89 "parser.y"
                                                { (yyval.ival) = day_from_monthday
								((yyvsp[0].ival), (yyvsp[-2].ival)); }
#line 1532 "y.tab.c"
    break;

  case 36: /* reldate: pexpr '.' month '.'  */
#line 91 "parser.y"
                                                { (yyval.ival) = day_from_monthday
								((yyvsp[-1].ival), (yyvsp[-3].ival)); }
#line 1539 "y.tab.c"
    break;

  case 37: /* reldate: month '/' pexpr  */
#line 93 "parser.y"
                                                { (yyval.ival) = day_from_monthday
								((yyvsp[-2].ival), (yyvsp[0].ival)); }
#line 1546 "y.tab.c"
    break;

  case 38: /* reldate: pexpr MONTH  */
#line 95 "parser.y"
                                                { (yyval.ival) = day_from_monthday
								((yyvsp[0].ival), (yyvsp[-1].ival)); }
#line 1553 "y.tab.c"
    break;

  case 39: /* reldate: MONTH pexpr  */
#line 97 "parser.y"
                                                { (yyval.ival) = day_from_monthday
								((yyvsp[-1].ival), (yyvsp[0].ival)); }
#line 1560 "y.tab.c"
    break;

  case 40: /* reldate: WDAY pexpr pexpr  */
#line 99 "parser.y"
                                                { (yyval.ival) = day_from_wday((yyvsp[0].ival), (yyvsp[-2].ival),
							(yyvsp[-1].ival) == -1 ? -1 : 0); }
#line 1567 "y.tab.c"
    break;

  case 41: /* reldate: pexpr WDAY IN month  */
#line 101 "parser.y"
                                                { int d=day_from_monthday
							((yyvsp[0].ival) + ((yyvsp[-3].ival) == 999),1);
						  (yyval.ival) = (yyvsp[-3].ival) == 999
						   ? day_from_wday(d,(yyvsp[-2].ival),-1)
						   : day_from_wday(d,(yyvsp[-2].ival),(yyvsp[-3].ival)-1);}
#line 1577 "y.tab.c"
    break;

  case 44: /* expr: pexpr  */
#line 110 "parser.y"
                                                { (yyval.ival) = (yyvsp[0].ival); }
#line 1583 "y.tab.c"
    break;

  case 45: /* expr: expr OR expr  */
#line 111 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) || (yyvsp[0].ival); }
#line 1589 "y.tab.c"
    break;

  case 46: /* expr: expr AND expr  */
#line 112 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) && (yyvsp[0].ival); }
#line 1595 "y.tab.c"
    break;

  case 47: /* expr: expr EQ_ expr  */
#line 113 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) == (yyvsp[0].ival); }
#line 1601 "y.tab.c"
    break;

  case 48: /* expr: expr NE expr  */
#line 114 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) != (yyvsp[0].ival); }
#line 1607 "y.tab.c"
    break;

  case 49: /* expr: expr LE expr  */
#line 115 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) <= (yyvsp[0].ival); }
#line 1613 "y.tab.c"
    break;

  case 50: /* expr: expr GE expr  */
#line 116 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) >= (yyvsp[0].ival); }
#line 1619 "y.tab.c"
    break;

  case 51: /* expr: expr LT expr  */
#line 117 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) <  (yyvsp[0].ival); }
#line 1625 "y.tab.c"
    break;

  case 52: /* expr: expr GT expr  */
#line 118 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) >  (yyvsp[0].ival); }
#line 1631 "y.tab.c"
    break;

  case 53: /* expr: expr '+' expr  */
#line 119 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) +  (yyvsp[0].ival); }
#line 1637 "y.tab.c"
    break;

  case 54: /* expr: expr '-' expr  */
#line 120 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) -  (yyvsp[0].ival); }
#line 1643 "y.tab.c"
    break;

  case 55: /* expr: expr '*' expr  */
#line 121 "parser.y"
                                                { (yyval.ival) = (yyvsp[-2].ival) *  (yyvsp[0].ival); }
#line 1649 "y.tab.c"
    break;

  case 56: /* expr: expr '/' expr  */
#line 122 "parser.y"
                                                { (yyval.ival) = (yyvsp[0].ival) ?  (yyvsp[-2].ival) / (yyvsp[0].ival) : 0; }
#line 1655 "y.tab.c"
    break;

  case 57: /* expr: expr '%' expr  */
#line 123 "parser.y"
                                                { (yyval.ival) = (yyvsp[0].ival) ?  (yyvsp[-2].ival) % (yyvsp[0].ival) : 0; }
#line 1661 "y.tab.c"
    break;

  case 58: /* expr: expr '?' expr ':' expr  */
#line 124 "parser.y"
                                                { (yyval.ival) = (yyvsp[-4].ival) ?  (yyvsp[-2].ival) : (yyvsp[0].ival); }
#line 1667 "y.tab.c"
    break;

  case 59: /* expr: '!' expr  */
#line 125 "parser.y"
                                                { (yyval.ival) = !(yyvsp[0].ival); }
#line 1673 "y.tab.c"
    break;

  case 60: /* expr: '[' reldate ']'  */
#line 126 "parser.y"
                                                { (yyval.ival) = (yyvsp[-1].ival); }
#line 1679 "y.tab.c"
    break;

  case 61: /* pexpr: '(' expr ')'  */
#line 129 "parser.y"
                                                { (yyval.ival) = (yyvsp[-1].ival); }
#line 1685 "y.tab.c"
    break;

  case 62: /* pexpr: number  */
#line 130 "parser.y"
                                                { (yyval.ival) = (yyvsp[0].ival); }
#line 1691 "y.tab.c"
    break;

  case 64: /* number: '-' NUMBER  */
#line 134 "parser.y"
                                                { (yyval.ival) = -(yyvsp[0].ival); }
#line 1697 "y.tab.c"
    break;

  case 65: /* number: CYEAR  */
#line 135 "parser.y"
                                                { (yyval.ival) = parse_year; }
#line 1703 "y.tab.c"
    break;

  case 66: /* number: LEAPYEAR pexpr  */
#line 136 "parser.y"
                                                { (yyval.ival) = !(((yyvsp[0].ival)) & 3); }
#line 1709 "y.tab.c"
    break;


#line 1713 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 138 "parser.y"

