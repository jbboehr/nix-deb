/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Skeleton implementation for Bison GLR parsers in C

   Copyright (C) 2002-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* C GLR parser skeleton written by Paul Hilfinger.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 1






/* First part of user declarations.  */
#line 51 "src/libexpr/parser.y" /* glr.c:240  */


#include "parser-tab.hh"
#include "lexer-tab.hh"

YY_DECL;

using namespace nix;


namespace nix {


static void dupAttr(const AttrPath & attrPath, const Pos & pos, const Pos & prevPos)
{
    throw ParseError(format("attribute ‘%1%’ at %2% already defined at %3%")
        % showAttrPath(attrPath) % pos % prevPos);
}


static void dupAttr(Symbol attr, const Pos & pos, const Pos & prevPos)
{
    throw ParseError(format("attribute ‘%1%’ at %2% already defined at %3%")
        % attr % pos % prevPos);
}


static void addAttr(ExprAttrs * attrs, AttrPath & attrPath,
    Expr * e, const Pos & pos)
{
    AttrPath::iterator i;
    // All attrpaths have at least one attr
    assert(!attrPath.empty());
    for (i = attrPath.begin(); i + 1 < attrPath.end(); i++) {
        if (i->symbol.set()) {
            ExprAttrs::AttrDefs::iterator j = attrs->attrs.find(i->symbol);
            if (j != attrs->attrs.end()) {
                if (!j->second.inherited) {
                    ExprAttrs * attrs2 = dynamic_cast<ExprAttrs *>(j->second.e);
                    if (!attrs2) dupAttr(attrPath, pos, j->second.pos);
                    attrs = attrs2;
                } else
                    dupAttr(attrPath, pos, j->second.pos);
            } else {
                ExprAttrs * nested = new ExprAttrs;
                attrs->attrs[i->symbol] = ExprAttrs::AttrDef(nested, pos);
                attrs = nested;
            }
        } else {
            ExprAttrs *nested = new ExprAttrs;
            attrs->dynamicAttrs.push_back(ExprAttrs::DynamicAttrDef(i->expr, nested, pos));
            attrs = nested;
        }
    }
    if (i->symbol.set()) {
        ExprAttrs::AttrDefs::iterator j = attrs->attrs.find(i->symbol);
        if (j != attrs->attrs.end()) {
            dupAttr(attrPath, pos, j->second.pos);
        } else {
            attrs->attrs[i->symbol] = ExprAttrs::AttrDef(e, pos);
            e->setName(i->symbol);
        }
    } else {
        attrs->dynamicAttrs.push_back(ExprAttrs::DynamicAttrDef(i->expr, e, pos));
    }
}


static void addFormal(const Pos & pos, Formals * formals, const Formal & formal)
{
    if (formals->argNames.find(formal.name) != formals->argNames.end())
        throw ParseError(format("duplicate formal function argument ‘%1%’ at %2%")
            % formal.name % pos);
    formals->formals.push_front(formal);
    formals->argNames.insert(formal.name);
}


static Expr * stripIndentation(const Pos & pos, SymbolTable & symbols, vector<Expr *> & es)
{
    if (es.empty()) return new ExprString(symbols.create(""));

    /* Figure out the minimum indentation.  Note that by design
       whitespace-only final lines are not taken into account.  (So
       the " " in "\n ''" is ignored, but the " " in "\n foo''" is.) */
    bool atStartOfLine = true; /* = seen only whitespace in the current line */
    unsigned int minIndent = 1000000;
    unsigned int curIndent = 0;
    for (auto & i : es) {
        ExprIndStr * e = dynamic_cast<ExprIndStr *>(i);
        if (!e) {
            /* Anti-quotations end the current start-of-line whitespace. */
            if (atStartOfLine) {
                atStartOfLine = false;
                if (curIndent < minIndent) minIndent = curIndent;
            }
            continue;
        }
        for (unsigned int j = 0; j < e->s.size(); ++j) {
            if (atStartOfLine) {
                if (e->s[j] == ' ')
                    curIndent++;
                else if (e->s[j] == '\n') {
                    /* Empty line, doesn't influence minimum
                       indentation. */
                    curIndent = 0;
                } else {
                    atStartOfLine = false;
                    if (curIndent < minIndent) minIndent = curIndent;
                }
            } else if (e->s[j] == '\n') {
                atStartOfLine = true;
                curIndent = 0;
            }
        }
    }

    /* Strip spaces from each line. */
    vector<Expr *> * es2 = new vector<Expr *>;
    atStartOfLine = true;
    unsigned int curDropped = 0;
    unsigned int n = es.size();
    for (vector<Expr *>::iterator i = es.begin(); i != es.end(); ++i, --n) {
        ExprIndStr * e = dynamic_cast<ExprIndStr *>(*i);
        if (!e) {
            atStartOfLine = false;
            curDropped = 0;
            es2->push_back(*i);
            continue;
        }

        string s2;
        for (unsigned int j = 0; j < e->s.size(); ++j) {
            if (atStartOfLine) {
                if (e->s[j] == ' ') {
                    if (curDropped++ >= minIndent)
                        s2 += e->s[j];
                }
                else if (e->s[j] == '\n') {
                    curDropped = 0;
                    s2 += e->s[j];
                } else {
                    atStartOfLine = false;
                    curDropped = 0;
                    s2 += e->s[j];
                }
            } else {
                s2 += e->s[j];
                if (e->s[j] == '\n') atStartOfLine = true;
            }
        }

        /* Remove the last line if it is empty and consists only of
           spaces. */
        if (n == 1) {
            string::size_type p = s2.find_last_of('\n');
            if (p != string::npos && s2.find_first_not_of(' ', p + 1) == string::npos)
                s2 = string(s2, 0, p + 1);
        }

        es2->push_back(new ExprString(symbols.create(s2)));
    }

    /* If this is a single string, then don't do a concatenation. */
    return es2->size() == 1 && dynamic_cast<ExprString *>((*es2)[0]) ? (*es2)[0] : new ExprConcatStrings(pos, true, es2);
}


static inline Pos makeCurPos(const YYLTYPE & loc, ParseData * data)
{
    return Pos(data->path, loc.first_line, loc.first_column);
}

#define CUR_POS makeCurPos(*yylocp, data)


}


void yyerror(YYLTYPE * loc, yyscan_t scanner, ParseData * data, const char * error)
{
    data->error = (format("%1%, at %2%")
        % error % makeCurPos(*loc, data)).str();
}



#line 242 "src/libexpr/parser-tab.cc" /* glr.c:240  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

#include "parser-tab.hh"

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;

/* Copy the second part of user declarations.  */

#line 275 "src/libexpr/parser-tab.cc" /* glr.c:263  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef YYFREE
# define YYFREE free
#endif
#ifndef YYMALLOC
# define YYMALLOC malloc
#endif
#ifndef YYREALLOC
# define YYREALLOC realloc
#endif

#define YYSIZEMAX ((size_t) -1)

#ifdef __cplusplus
   typedef bool yybool;
#else
   typedef unsigned char yybool;
#endif
#define yytrue 1
#define yyfalse 0

#ifndef YYSETJMP
# include <setjmp.h>
# define YYJMP_BUF jmp_buf
# define YYSETJMP(Env) setjmp (Env)
/* Pacify clang.  */
# define YYLONGJMP(Env, Val) (longjmp (Env, Val), YYASSERT (0))
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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


#ifndef YYASSERT
# define YYASSERT(Condition) ((void) ((Condition) || (abort (), 0)))
#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  52
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   332

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  58
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  84
/* YYNRULES -- Number of states.  */
#define YYNSTATES  168
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 7
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYTRANSLATE(X) -- Bison symbol number corresponding to X.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   291

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    49,    51,     2,     2,     2,     2,     2,
      52,    53,    39,    37,    57,    38,    50,    40,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    44,    48,
      31,    56,    32,    42,    47,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    54,     2,    55,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    45,     2,    46,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    33,    34,    35,    36,
      41,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   289,   289,   291,   294,   296,   298,   300,   302,   304,
     306,   312,   316,   317,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   336,   337,
     338,   339,   340,   344,   346,   350,   352,   356,   358,   362,
     368,   369,   370,   373,   374,   375,   382,   383,   386,   388,
     390,   392,   396,   397,   398,   402,   403,   404,   405,   413,
     414,   415,   419,   420,   429,   438,   442,   443,   453,   457,
     458,   467,   468,   480,   481,   485,   486,   490,   491,   495,
     497,   500,   501,   506,   507
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "ATTRPATH", "STR", "IND_STR",
  "INT", "PATH", "HPATH", "SPATH", "URI", "IF", "THEN", "ELSE", "ASSERT",
  "WITH", "LET", "IN", "REC", "INHERIT", "EQ", "NEQ", "AND", "OR", "IMPL",
  "OR_KW", "DOLLAR_CURLY", "IND_STRING_OPEN", "IND_STRING_CLOSE",
  "ELLIPSIS", "'<'", "'>'", "LEQ", "GEQ", "UPDATE", "NOT", "'+'", "'-'",
  "'*'", "'/'", "CONCAT", "'?'", "NEGATE", "':'", "'{'", "'}'", "'@'",
  "';'", "'!'", "'.'", "'\"'", "'('", "')'", "'['", "']'", "'='", "','",
  "$accept", "start", "expr", "expr_function", "expr_if", "expr_op",
  "expr_app", "expr_select", "expr_simple", "string_parts",
  "string_parts_interpolated", "ind_string_parts", "binds", "attrs",
  "attrpath", "attr", "string_attr", "expr_list", "formals", "formal", YY_NULLPTR
};
#endif

#define YYPACT_NINF -123
#define YYTABLE_NINF -66

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const short int yypact[] =
{
     134,    26,  -123,  -123,  -123,  -123,  -123,   134,   134,   134,
     -33,   -25,  -123,   149,     4,   149,    60,   134,  -123,    49,
    -123,  -123,  -123,   201,   199,  -123,   -13,   134,   -23,    48,
      28,    30,  -123,     6,  -123,    52,  -123,   -33,  -123,  -123,
      32,  -123,    18,    34,    36,   233,    85,   134,    69,    86,
      79,     8,  -123,   149,   149,   149,   149,   149,   149,   149,
     149,   149,   149,   149,   149,   149,   149,   149,   170,  -123,
    -123,   170,  -123,    11,   134,   134,   134,    63,  -123,   134,
      70,  -123,   134,    60,   -21,  -123,  -123,   144,  -123,   134,
    -123,   134,  -123,   131,    11,   134,    87,  -123,  -123,   134,
    -123,  -123,  -123,   268,   268,   290,   246,   224,    88,    88,
      88,    88,    88,    76,    76,    44,    44,    44,    84,    22,
      92,   126,  -123,  -123,  -123,  -123,   134,    20,   102,   103,
     170,   134,  -123,   109,  -123,   134,   158,  -123,   117,  -123,
     119,   199,   123,   134,   121,  -123,  -123,  -123,  -123,  -123,
    -123,  -123,   128,  -123,  -123,   136,  -123,  -123,  -123,   134,
    -123,  -123,  -123,   134,  -123,   166,  -123,  -123
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,    39,    40,    43,    44,    45,    46,     0,     0,     0,
      65,     0,    61,     0,    65,     0,    54,     0,    78,     0,
       2,     3,    11,    13,    32,    34,    38,     0,     0,     0,
       0,     0,    65,     0,    65,     0,    39,     0,    65,    15,
      83,    82,     0,     0,    80,    14,    52,     0,     0,    53,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    33,
      37,     0,     4,    81,     0,     0,     0,     0,    73,     0,
      68,    74,     0,    54,     0,    71,    72,     0,    59,     0,
      42,     0,    50,     0,    81,     0,     0,    41,    55,     0,
      47,    51,    77,    16,    17,    22,    23,    24,    18,    20,
      19,    21,    25,    27,    28,    29,    30,    31,    26,    35,
       0,     0,     8,     9,    48,    10,     0,     0,     0,     0,
       0,     0,    49,     0,    84,     0,     0,    79,     0,    57,
       0,     0,     0,     0,     0,    63,    66,    67,    76,    75,
      69,    70,     0,    60,     5,     0,    58,    56,    36,     0,
      12,    68,    62,     0,     7,     0,     6,    64
};

  /* YYPGOTO[NTERM-NUM].  */
static const signed char yypgoto[] =
{
    -123,  -123,    -7,   -24,  -123,    41,  -123,   -20,  -123,    99,
    -123,  -123,    -4,    38,   113,  -122,   -88,  -123,    37,  -123
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const signed char yydefgoto[] =
{
      -1,    19,    20,    21,    22,    23,    24,    25,    26,    48,
      49,    35,    42,   127,    84,    85,    86,    51,    43,    44
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const short int yytable[] =
{
      29,    30,    31,    72,    69,   146,    33,    40,   150,    78,
      50,    36,    32,    70,    40,     2,     3,     4,     5,     6,
      34,    78,    73,    78,    79,    37,    80,    11,    77,   130,
      87,   102,    81,    82,    41,   131,    12,    71,    80,   147,
      96,    41,   151,   146,    81,    82,    81,    82,   141,    52,
     -65,   122,   123,    38,    39,   125,    45,    83,    88,    16,
      17,    74,    18,   101,    92,    46,    78,   121,   145,    83,
      27,    83,   130,    28,    91,   128,    75,   147,    76,    89,
      93,    90,   133,    80,   134,    67,    68,    47,   138,    81,
      82,    98,   140,    94,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   124,
     120,   154,    95,    99,    83,    65,    66,    67,    68,   144,
      97,   158,   126,    62,   152,    63,    64,    65,    66,    67,
      68,   137,   100,   139,   130,   164,   160,     1,   142,   166,
     143,     2,     3,     4,     5,     6,     7,    78,   148,     8,
       9,    10,    36,    11,   149,   153,     2,     3,     4,     5,
       6,   155,    12,   156,    80,   157,    37,   159,    11,    78,
      81,    82,    13,    78,   161,   135,   162,    12,   136,    14,
     163,   118,   129,    15,   119,    16,    17,    13,    18,     0,
     132,     0,    81,    82,    38,    83,    81,    82,    15,   165,
      16,    17,    36,    18,     0,     0,     2,     3,     4,     5,
       6,     0,     0,     0,   167,     0,    37,    83,    11,     0,
       0,    83,    53,    54,    55,    56,    57,    12,     0,     0,
       0,     0,    58,    59,    60,    61,    62,     0,    63,    64,
      65,    66,    67,    68,    38,    53,    54,    55,    56,   -66,
      16,    17,     0,    18,     0,    58,    59,    60,    61,    62,
       0,    63,    64,    65,    66,    67,    68,    53,    54,    55,
      63,    64,    65,    66,    67,    68,     0,    58,    59,    60,
      61,    62,     0,    63,    64,    65,    66,    67,    68,   -66,
     -66,     0,     0,     0,     0,     0,     0,     0,     0,    58,
      59,    60,    61,    62,     0,    63,    64,    65,    66,    67,
      68,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,    58,    59,    60,    61,    62,     0,    63,    64,    65,
      66,    67,    68
};

static const short int yycheck[] =
{
       7,     8,     9,    27,    24,   127,    10,     3,   130,     3,
      17,     3,    45,    26,     3,     7,     8,     9,    10,    11,
      45,     3,    45,     3,    18,    17,    20,    19,    32,    50,
      34,    51,    26,    27,    30,    56,    28,    50,    20,   127,
      47,    30,   130,   165,    26,    27,    26,    27,    26,     0,
      46,    75,    76,    45,    13,    79,    15,    51,     6,    51,
      52,    13,    54,    55,    46,     5,     3,    74,    48,    51,
      44,    51,    50,    47,    42,    82,    48,   165,    48,    27,
      46,    29,    89,    20,    91,    41,    42,    27,    95,    26,
      27,     5,    99,    57,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    46,
      73,   135,    27,    27,    51,    39,    40,    41,    42,   126,
      51,   141,    52,    35,   131,    37,    38,    39,    40,    41,
      42,    94,    53,    46,    50,   159,   143,     3,    46,   163,
      14,     7,     8,     9,    10,    11,    12,     3,    46,    15,
      16,    17,     3,    19,    51,    46,     7,     8,     9,    10,
      11,     3,    28,    46,    20,    46,    17,    44,    19,     3,
      26,    27,    38,     3,    53,    44,    48,    28,    47,    45,
      44,    68,    83,    49,    71,    51,    52,    38,    54,    -1,
      46,    -1,    26,    27,    45,    51,    26,    27,    49,   161,
      51,    52,     3,    54,    -1,    -1,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    48,    -1,    17,    51,    19,    -1,
      -1,    51,    21,    22,    23,    24,    25,    28,    -1,    -1,
      -1,    -1,    31,    32,    33,    34,    35,    -1,    37,    38,
      39,    40,    41,    42,    45,    21,    22,    23,    24,    25,
      51,    52,    -1,    54,    -1,    31,    32,    33,    34,    35,
      -1,    37,    38,    39,    40,    41,    42,    21,    22,    23,
      37,    38,    39,    40,    41,    42,    -1,    31,    32,    33,
      34,    35,    -1,    37,    38,    39,    40,    41,    42,    21,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    -1,    37,    38,    39,    40,    41,
      42,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    -1,    37,    38,    39,
      40,    41,    42
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     7,     8,     9,    10,    11,    12,    15,    16,
      17,    19,    28,    38,    45,    49,    51,    52,    54,    59,
      60,    61,    62,    63,    64,    65,    66,    44,    47,    60,
      60,    60,    45,    70,    45,    69,     3,    17,    45,    63,
       3,    30,    70,    76,    77,    63,     5,    27,    67,    68,
      60,    75,     0,    21,    22,    23,    24,    25,    31,    32,
      33,    34,    35,    37,    38,    39,    40,    41,    42,    65,
      26,    50,    61,    45,    13,    48,    48,    70,     3,    18,
      20,    26,    27,    51,    72,    73,    74,    70,     6,    27,
      29,    42,    46,    46,    57,    27,    60,    51,     5,    27,
      53,    55,    65,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    72,    72,
      76,    60,    61,    61,    46,    61,    52,    71,    60,    67,
      50,    56,    46,    60,    60,    44,    47,    76,    60,    46,
      60,    26,    46,    14,    60,    48,    73,    74,    46,    51,
      73,    74,    60,    46,    61,     3,    46,    46,    65,    44,
      60,    53,    48,    44,    61,    71,    61,    48
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    58,    59,    60,    61,    61,    61,    61,    61,    61,
      61,    61,    62,    62,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    64,    64,    65,    65,    65,    65,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    67,    67,    67,    68,    68,    68,    68,    69,
      69,    69,    70,    70,    70,    70,    71,    71,    71,    72,
      72,    72,    72,    73,    73,    74,    74,    75,    75,    76,
      76,    76,    76,    77,    77
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     5,     7,     7,     4,     4,
       4,     1,     6,     1,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     1,     2,     1,     3,     5,     2,     1,     1,
       1,     3,     3,     1,     1,     1,     1,     3,     4,     4,
       3,     3,     1,     1,     0,     2,     4,     3,     4,     2,
       4,     0,     5,     4,     7,     0,     2,     2,     0,     3,
       3,     1,     1,     1,     1,     3,     3,     2,     0,     3,
       1,     0,     1,     1,     3
};


/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const unsigned char yydprec[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0
};

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const unsigned char yymerger[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0
};

/* YYIMMEDIATE[RULE-NUM] -- True iff rule #RULE-NUM is not to be deferred, as
   in the case of predicates.  */
static const yybool yyimmediate[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0
};

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const unsigned char yyconflp[] =
{
       0,     0,     0,     0,     0,     0,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0
};

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short int yyconfl[] =
{
       0,    65,     0,    81,     0
};

/* Error token number */
#define YYTERROR 1


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

# define YYRHSLOC(Rhs, K) ((Rhs)[K].yystate.yyloc)



#undef yynerrs
#define yynerrs (yystackp->yyerrcnt)
#undef yychar
#define yychar (yystackp->yyrawchar)
#undef yylval
#define yylval (yystackp->yyval)
#undef yylloc
#define yylloc (yystackp->yyloc)


static const int YYEOF = 0;
static const int YYEMPTY = -2;

typedef enum { yyok, yyaccept, yyabort, yyerr } YYRESULTTAG;

#define YYCHK(YYE)                              \
  do {                                          \
    YYRESULTTAG yychk_flag = YYE;               \
    if (yychk_flag != yyok)                     \
      return yychk_flag;                        \
  } while (0)

#if YYDEBUG

# ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YYDPRINTF(Args)                        \
  do {                                          \
    if (yydebug)                                \
      YYFPRINTF Args;                           \
  } while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void * scanner, nix::ParseData * data)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (data);
  if (!yyvaluep)
    return;
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, void * scanner, nix::ParseData * data)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, data);
  YYFPRINTF (yyoutput, ")");
}

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                  \
  do {                                                                  \
    if (yydebug)                                                        \
      {                                                                 \
        YYFPRINTF (stderr, "%s ", Title);                               \
        yy_symbol_print (stderr, Type, Value, Location, scanner, data);        \
        YYFPRINTF (stderr, "\n");                                       \
      }                                                                 \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

struct yyGLRStack;
static void yypstack (struct yyGLRStack* yystackp, size_t yyk)
  YY_ATTRIBUTE_UNUSED;
static void yypdumpstack (struct yyGLRStack* yystackp)
  YY_ATTRIBUTE_UNUSED;

#else /* !YYDEBUG */

# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)

#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
#  define YYSTACKEXPANDABLE 1
#endif

#if YYSTACKEXPANDABLE
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyexpandGLRStack (Yystack);                       \
  } while (0)
#else
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyMemoryExhausted (Yystack);                      \
  } while (0)
#endif


#if YYERROR_VERBOSE

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static size_t
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return strlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* !YYERROR_VERBOSE */

/** State numbers, as in LALR(1) machine */
typedef int yyStateNum;

/** Rule numbers, as in LALR(1) machine */
typedef int yyRuleNum;

/** Grammar symbol */
typedef int yySymbol;

/** Item references, as in LALR(1) machine */
typedef short int yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState {
  /** Type tag: always true.  */
  yybool yyisState;
  /** Type tag for yysemantics.  If true, yysval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yyStateNum yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the last token produced by my symbol */
  size_t yyposn;
  union {
    /** First in a chain of alternative reductions producing the
     *  non-terminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yysval;
  } yysemantics;
  /** Source location for this state.  */
  YYLTYPE yyloc;
};

struct yyGLRStateSet {
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != YYEMPTY.  */
  yybool* yylookaheadNeeds;
  size_t yysize, yycapacity;
};

struct yySemanticOption {
  /** Type tag: always false.  */
  yybool yyisState;
  /** Rule number for this reduction */
  yyRuleNum yyrule;
  /** The last RHS state in the list of states to be reduced.  */
  yyGLRState* yystate;
  /** The lookahead for this reduction.  */
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;
  /** Next sibling in chain of options.  To facilitate merging,
   *  options are chained in decreasing order by address.  */
  yySemanticOption* yynext;
};

/** Type of the items in the GLR stack.  The yyisState field
 *  indicates which item of the union is valid.  */
union yyGLRStackItem {
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack {
  int yyerrState;
  /* To compute the location of the error token.  */
  yyGLRStackItem yyerror_range[3];

  int yyerrcnt;
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;

  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  size_t yyspaceLeft;
  yyGLRState* yysplitPoint;
  yyGLRState* yylastDeleted;
  yyGLRStateSet yytops;
};

#if YYSTACKEXPANDABLE
static void yyexpandGLRStack (yyGLRStack* yystackp);
#endif

static _Noreturn void
yyFail (yyGLRStack* yystackp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data, const char* yymsg)
{
  if (yymsg != YY_NULLPTR)
    yyerror (yylocp, scanner, data, yymsg);
  YYLONGJMP (yystackp->yyexception_buffer, 1);
}

static _Noreturn void
yyMemoryExhausted (yyGLRStack* yystackp)
{
  YYLONGJMP (yystackp->yyexception_buffer, 2);
}

#if YYDEBUG || YYERROR_VERBOSE
/** A printable representation of TOKEN.  */
static inline const char*
yytokenName (yySymbol yytoken)
{
  if (yytoken == YYEMPTY)
    return "";

  return yytname[yytoken];
}
#endif

/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin (yyGLRStackItem *, int, int) YY_ATTRIBUTE_UNUSED;
static void
yyfillin (yyGLRStackItem *yyvsp, int yylow0, int yylow1)
{
  int i;
  yyGLRState *s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0-1; i >= yylow1; i -= 1)
    {
#if YYDEBUG
      yyvsp[i].yystate.yylrState = s->yylrState;
#endif
      yyvsp[i].yystate.yyresolved = s->yyresolved;
      if (s->yyresolved)
        yyvsp[i].yystate.yysemantics.yysval = s->yysemantics.yysval;
      else
        /* The effect of using yysval or yyloc (in an immediate rule) is
         * undefined.  */
        yyvsp[i].yystate.yysemantics.yyfirstVal = YY_NULLPTR;
      yyvsp[i].yystate.yyloc = s->yyloc;
      s = yyvsp[i].yystate.yypred = s->yypred;
    }
}

/* Do nothing if YYNORMAL or if *YYLOW <= YYLOW1.  Otherwise, fill in
 * YYVSP[YYLOW1 .. *YYLOW-1] as in yyfillin and set *YYLOW = YYLOW1.
 * For convenience, always return YYLOW1.  */
static inline int yyfill (yyGLRStackItem *, int *, int, yybool)
     YY_ATTRIBUTE_UNUSED;
static inline int
yyfill (yyGLRStackItem *yyvsp, int *yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
    {
      yyfillin (yyvsp, *yylow, yylow1);
      *yylow = yylow1;
    }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT.  */
static YYRESULTTAG
yyuserAction (yyRuleNum yyn, size_t yyrhslen, yyGLRStackItem* yyvsp,
              yyGLRStack* yystackp,
              YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  yybool yynormal YY_ATTRIBUTE_UNUSED = (yystackp->yysplitPoint == YY_NULLPTR);
  int yylow;
  YYUSE (yyvalp);
  YYUSE (yylocp);
  YYUSE (scanner);
  YYUSE (data);
  YYUSE (yyrhslen);
# undef yyerrok
# define yyerrok (yystackp->yyerrState = 0)
# undef YYACCEPT
# define YYACCEPT return yyaccept
# undef YYABORT
# define YYABORT return yyabort
# undef YYERROR
# define YYERROR return yyerrok, yyerr
# undef YYRECOVERING
# define YYRECOVERING() (yystackp->yyerrState != 0)
# undef yyclearin
# define yyclearin (yychar = YYEMPTY)
# undef YYFILL
# define YYFILL(N) yyfill (yyvsp, &yylow, N, yynormal)
# undef YYBACKUP
# define YYBACKUP(Token, Value)                                              \
  return yyerror (yylocp, scanner, data, YY_("syntax error: cannot back up")),     \
         yyerrok, yyerr

  yylow = 1;
  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL (1-yyrhslen)].yystate.yysemantics.yysval;
  YYLLOC_DEFAULT ((*yylocp), (yyvsp - yyrhslen), yyrhslen);
  yystackp->yyerror_range[1].yystate.yyloc = *yylocp;

  switch (yyn)
    {
        case 2:
#line 289 "src/libexpr/parser.y" /* glr.c:816  */
    { data->result = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e); }
#line 1264 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 4:
#line 295 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.id)), false, 0, (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1270 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 5:
#line 297 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create(""), true, (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1276 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 6:
#line 299 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.id)), true, (((yyGLRStackItem const *)yyvsp)[YYFILL (-5)].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1282 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 7:
#line 301 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprLambda(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (-6)].yystate.yysemantics.yysval.id)), true, (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.formals), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1288 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 8:
#line 303 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprAssert(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1294 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 9:
#line 305 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprWith(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1300 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 10:
#line 307 "src/libexpr/parser.y" /* glr.c:816  */
    { if (!(((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrs)->dynamicAttrs.empty())
        throw ParseError(format("dynamic attributes not allowed in let at %1%")
            % CUR_POS);
      ((*yyvalp).e) = new ExprLet((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrs), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e));
    }
#line 1310 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 12:
#line 316 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprIf((((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1316 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 14:
#line 321 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpNot((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1322 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 15:
#line 322 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__sub")), new ExprInt(0)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1328 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 16:
#line 323 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpEq((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1334 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 17:
#line 324 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpNEq((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1340 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 18:
#line 325 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1346 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 19:
#line 326 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpNot(new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e))); }
#line 1352 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 20:
#line 327 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)); }
#line 1358 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 21:
#line 328 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpNot(new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__lessThan")), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e))); }
#line 1364 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 22:
#line 329 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpAnd(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1370 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 23:
#line 330 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpOr(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1376 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 24:
#line 331 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpImpl(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1382 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 25:
#line 332 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpUpdate(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1388 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 26:
#line 333 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpHasAttr((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.attrNames)); }
#line 1394 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 27:
#line 335 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprConcatStrings(CUR_POS, false, new vector<Expr *>({(((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)})); }
#line 1400 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 28:
#line 336 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__sub")), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1406 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 29:
#line 337 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__mul")), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1412 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 30:
#line 338 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, new ExprApp(new ExprVar(data->symbols.create("__div")), (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1418 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 31:
#line 339 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprOpConcatLists(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1424 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 33:
#line 345 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1430 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 34:
#line 346 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e); }
#line 1436 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 35:
#line 351 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.attrNames), 0); }
#line 1442 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 36:
#line 353 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.e), *(((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrNames), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1448 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 37:
#line 357 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprApp(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e), new ExprVar(CUR_POS, data->symbols.create("or"))); }
#line 1454 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 38:
#line 358 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e); }
#line 1460 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 39:
#line 362 "src/libexpr/parser.y" /* glr.c:816  */
    {
      if (strcmp((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id), "__curPos") == 0)
          ((*yyvalp).e) = new ExprPos(CUR_POS);
      else
          ((*yyvalp).e) = new ExprVar(CUR_POS, data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id)));
  }
#line 1471 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 40:
#line 368 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprInt((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.n)); }
#line 1477 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 41:
#line 369 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e); }
#line 1483 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 42:
#line 370 "src/libexpr/parser.y" /* glr.c:816  */
    {
      ((*yyvalp).e) = stripIndentation(CUR_POS, data->symbols, *(((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.string_parts));
  }
#line 1491 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 43:
#line 373 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprPath(absPath((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.path), data->basePath)); }
#line 1497 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 44:
#line 374 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprPath(getEnv("HOME", "") + string{(((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.path) + 1}); }
#line 1503 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 45:
#line 375 "src/libexpr/parser.y" /* glr.c:816  */
    {
      string path((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.path) + 1, strlen((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.path)) - 2);
      ((*yyvalp).e) = new ExprApp(CUR_POS,
          new ExprApp(new ExprVar(data->symbols.create("__findFile")),
              new ExprVar(data->symbols.create("__nixPath"))),
          new ExprString(data->symbols.create(path)));
  }
#line 1515 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 46:
#line 382 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprString(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.uri))); }
#line 1521 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 47:
#line 383 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e); }
#line 1527 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 48:
#line 387 "src/libexpr/parser.y" /* glr.c:816  */
    { (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrs)->recursive = true; ((*yyvalp).e) = new ExprSelect(noPos, (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrs), data->symbols.create("body")); }
#line 1533 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 49:
#line 389 "src/libexpr/parser.y" /* glr.c:816  */
    { (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrs)->recursive = true; ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrs); }
#line 1539 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 50:
#line 391 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrs); }
#line 1545 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 51:
#line 392 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.list); }
#line 1551 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 53:
#line 397 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprConcatStrings(CUR_POS, true, (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.string_parts)); }
#line 1557 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 54:
#line 398 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = new ExprString(data->symbols.create("")); }
#line 1563 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 55:
#line 402 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1569 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 56:
#line 403 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e)); }
#line 1575 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 57:
#line 404 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = new vector<Expr *>; ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e)); }
#line 1581 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 58:
#line 405 "src/libexpr/parser.y" /* glr.c:816  */
    {
      ((*yyvalp).string_parts) = new vector<Expr *>;
      ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.e));
      ((*yyvalp).string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e));
    }
#line 1591 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 59:
#line 413 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1597 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 60:
#line 414 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.string_parts); (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.string_parts)->push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e)); }
#line 1603 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 61:
#line 415 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).string_parts) = new vector<Expr *>; }
#line 1609 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 62:
#line 419 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-4)].yystate.yysemantics.yysval.attrs); addAttr(((*yyvalp).attrs), *(((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.attrNames), (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e), makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yyloc), data)); }
#line 1615 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 63:
#line 421 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.attrs);
      for (auto & i : *(((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrNames)) {
          if (((*yyvalp).attrs)->attrs.find(i.symbol) != ((*yyvalp).attrs)->attrs.end())
              dupAttr(i.symbol, makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yyloc), data), ((*yyvalp).attrs)->attrs[i.symbol].pos);
          Pos pos = makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yyloc), data);
          ((*yyvalp).attrs)->attrs[i.symbol] = ExprAttrs::AttrDef(new ExprVar(CUR_POS, i.symbol), pos, true);
      }
    }
#line 1628 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 64:
#line 430 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrs) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-6)].yystate.yysemantics.yysval.attrs);
      /* !!! Should ensure sharing of the expression in $4. */
      for (auto & i : *(((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrNames)) {
          if (((*yyvalp).attrs)->attrs.find(i.symbol) != ((*yyvalp).attrs)->attrs.end())
              dupAttr(i.symbol, makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yyloc), data), ((*yyvalp).attrs)->attrs[i.symbol].pos);
          ((*yyvalp).attrs)->attrs[i.symbol] = ExprAttrs::AttrDef(new ExprSelect(CUR_POS, (((yyGLRStackItem const *)yyvsp)[YYFILL (-3)].yystate.yysemantics.yysval.e), i.symbol), makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yyloc), data));
      }
    }
#line 1641 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 65:
#line 438 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrs) = new ExprAttrs; }
#line 1647 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 66:
#line 442 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrNames); (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id)))); }
#line 1653 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 67:
#line 444 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.attrNames);
      ExprString * str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          throw ParseError(format("dynamic attributes not allowed in inherit at %1%")
              % makeCurPos((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yyloc), data));
    }
#line 1667 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 68:
#line 453 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = new AttrPath; }
#line 1673 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 69:
#line 457 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrNames); (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id)))); }
#line 1679 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 70:
#line 459 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.attrNames);
      ExprString * str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          ((*yyvalp).attrNames)->push_back(AttrName((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)));
    }
#line 1692 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 71:
#line 467 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = new vector<AttrName>; ((*yyvalp).attrNames)->push_back(AttrName(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id)))); }
#line 1698 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 72:
#line 469 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).attrNames) = new vector<AttrName>;
      ExprString *str = dynamic_cast<ExprString *>((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e));
      if (str) {
          ((*yyvalp).attrNames)->push_back(AttrName(str->s));
          delete str;
      } else
          ((*yyvalp).attrNames)->push_back(AttrName((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)));
    }
#line 1711 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 73:
#line 480 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).id) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id); }
#line 1717 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 74:
#line 481 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).id) = "or"; }
#line 1723 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 75:
#line 485 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e); }
#line 1729 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 76:
#line 486 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).e) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.e); }
#line 1735 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 77:
#line 490 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).list) = (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.list); (((yyGLRStackItem const *)yyvsp)[YYFILL (-1)].yystate.yysemantics.yysval.list)->elems.push_back((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); /* !!! dangerous */ }
#line 1741 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 78:
#line 491 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).list) = new ExprList; }
#line 1747 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 79:
#line 496 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formals) = (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.formals); addFormal(CUR_POS, ((*yyvalp).formals), *(((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.formal)); }
#line 1753 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 80:
#line 498 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formals) = new Formals; addFormal(CUR_POS, ((*yyvalp).formals), *(((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.formal)); ((*yyvalp).formals)->ellipsis = false; }
#line 1759 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 81:
#line 500 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formals) = new Formals; ((*yyvalp).formals)->ellipsis = false; }
#line 1765 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 82:
#line 502 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formals) = new Formals; ((*yyvalp).formals)->ellipsis = true; }
#line 1771 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 83:
#line 506 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formal) = new Formal(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.id)), 0); }
#line 1777 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;

  case 84:
#line 507 "src/libexpr/parser.y" /* glr.c:816  */
    { ((*yyvalp).formal) = new Formal(data->symbols.create((((yyGLRStackItem const *)yyvsp)[YYFILL (-2)].yystate.yysemantics.yysval.id)), (((yyGLRStackItem const *)yyvsp)[YYFILL (0)].yystate.yysemantics.yysval.e)); }
#line 1783 "src/libexpr/parser-tab.cc" /* glr.c:816  */
    break;


#line 1787 "src/libexpr/parser-tab.cc" /* glr.c:816  */
      default: break;
    }

  return yyok;
# undef yyerrok
# undef YYABORT
# undef YYACCEPT
# undef YYERROR
# undef YYBACKUP
# undef yyclearin
# undef YYRECOVERING
}


static void
yyuserMerge (int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YYUSE (yy0);
  YYUSE (yy1);

  switch (yyn)
    {

      default: break;
    }
}

                              /* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, void * scanner, nix::ParseData * data)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (data);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/** Number of symbols composing the right hand side of rule #RULE.  */
static inline int
yyrhsLength (yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void
yydestroyGLRState (char const *yymsg, yyGLRState *yys, void * scanner, nix::ParseData * data)
{
  if (yys->yyresolved)
    yydestruct (yymsg, yystos[yys->yylrState],
                &yys->yysemantics.yysval, &yys->yyloc, scanner, data);
  else
    {
#if YYDEBUG
      if (yydebug)
        {
          if (yys->yysemantics.yyfirstVal)
            YYFPRINTF (stderr, "%s unresolved", yymsg);
          else
            YYFPRINTF (stderr, "%s incomplete", yymsg);
          YY_SYMBOL_PRINT ("", yystos[yys->yylrState], YY_NULLPTR, &yys->yyloc);
        }
#endif

      if (yys->yysemantics.yyfirstVal)
        {
          yySemanticOption *yyoption = yys->yysemantics.yyfirstVal;
          yyGLRState *yyrh;
          int yyn;
          for (yyrh = yyoption->yystate, yyn = yyrhsLength (yyoption->yyrule);
               yyn > 0;
               yyrh = yyrh->yypred, yyn -= 1)
            yydestroyGLRState (yymsg, yyrh, scanner, data);
        }
    }
}

/** Left-hand-side symbol for rule #YYRULE.  */
static inline yySymbol
yylhsNonterm (yyRuleNum yyrule)
{
  return yyr1[yyrule];
}

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-123)))

/** True iff LR state YYSTATE has only a default reduction (regardless
 *  of token).  */
static inline yybool
yyisDefaultedState (yyStateNum yystate)
{
  return yypact_value_is_default (yypact[yystate]);
}

/** The default reduction for YYSTATE, assuming it has one.  */
static inline yyRuleNum
yydefaultAction (yyStateNum yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-66)))

/** Set *YYACTION to the action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *YYCONFLICTS to a pointer into yyconfl to a 0-terminated list
 *  of conflicting reductions.
 */
static inline void
yygetLRActions (yyStateNum yystate, int yytoken,
                int* yyaction, const short int** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yypact_value_is_default (yypact[yystate])
      || yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
    {
      *yyaction = -yydefact[yystate];
      *yyconflicts = yyconfl;
    }
  else if (! yytable_value_is_error (yytable[yyindex]))
    {
      *yyaction = yytable[yyindex];
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
  else
    {
      *yyaction = 0;
      *yyconflicts = yyconfl + yyconflp[yyindex];
    }
}

/** Compute post-reduction state.
 * \param yystate   the current state
 * \param yysym     the nonterminal to push on the stack
 */
static inline yyStateNum
yyLRgotoState (yyStateNum yystate, yySymbol yysym)
{
  int yyr = yypgoto[yysym - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yysym - YYNTOKENS];
}

static inline yybool
yyisShiftAction (int yyaction)
{
  return 0 < yyaction;
}

static inline yybool
yyisErrorAction (int yyaction)
{
  return yyaction == 0;
}

                                /* GLRStates */

/** Return a fresh GLRStackItem in YYSTACKP.  The item is an LR state
 *  if YYISSTATE, and otherwise a semantic option.  Callers should call
 *  YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 *  headroom.  */

static inline yyGLRStackItem*
yynewGLRStackItem (yyGLRStack* yystackp, yybool yyisState)
{
  yyGLRStackItem* yynewItem = yystackp->yynextFree;
  yystackp->yyspaceLeft -= 1;
  yystackp->yynextFree += 1;
  yynewItem->yystate.yyisState = yyisState;
  return yynewItem;
}

/** Add a new semantic action that will execute the action for rule
 *  YYRULE on the semantic values in YYRHS to the list of
 *  alternative actions for YYSTATE.  Assumes that YYRHS comes from
 *  stack #YYK of *YYSTACKP. */
static void
yyaddDeferredAction (yyGLRStack* yystackp, size_t yyk, yyGLRState* yystate,
                     yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption =
    &yynewGLRStackItem (yystackp, yyfalse)->yyoption;
  YYASSERT (!yynewOption->yyisState);
  yynewOption->yystate = yyrhs;
  yynewOption->yyrule = yyrule;
  if (yystackp->yytops.yylookaheadNeeds[yyk])
    {
      yynewOption->yyrawchar = yychar;
      yynewOption->yyval = yylval;
      yynewOption->yyloc = yylloc;
    }
  else
    yynewOption->yyrawchar = YYEMPTY;
  yynewOption->yynext = yystate->yysemantics.yyfirstVal;
  yystate->yysemantics.yyfirstVal = yynewOption;

  YY_RESERVE_GLRSTACK (yystackp);
}

                                /* GLRStacks */

/** Initialize YYSET to a singleton set containing an empty stack.  */
static yybool
yyinitStateSet (yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates = (yyGLRState**) YYMALLOC (16 * sizeof yyset->yystates[0]);
  if (! yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULLPTR;
  yyset->yylookaheadNeeds =
    (yybool*) YYMALLOC (16 * sizeof yyset->yylookaheadNeeds[0]);
  if (! yyset->yylookaheadNeeds)
    {
      YYFREE (yyset->yystates);
      return yyfalse;
    }
  return yytrue;
}

static void yyfreeStateSet (yyGLRStateSet* yyset)
{
  YYFREE (yyset->yystates);
  YYFREE (yyset->yylookaheadNeeds);
}

/** Initialize *YYSTACKP to a single empty stack, with total maximum
 *  capacity for all stacks of YYSIZE.  */
static yybool
yyinitGLRStack (yyGLRStack* yystackp, size_t yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems =
    (yyGLRStackItem*) YYMALLOC (yysize * sizeof yystackp->yynextFree[0]);
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;
  return yyinitStateSet (&yystackp->yytops);
}


#if YYSTACKEXPANDABLE
# define YYRELOC(YYFROMITEMS,YYTOITEMS,YYX,YYTYPE) \
  &((YYTOITEMS) - ((YYFROMITEMS) - (yyGLRStackItem*) (YYX)))->YYTYPE

/** If *YYSTACKP is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void
yyexpandGLRStack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem* yyp0, *yyp1;
  size_t yynewSize;
  size_t yyn;
  size_t yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted (yystackp);
  yynewSize = 2*yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems = (yyGLRStackItem*) YYMALLOC (yynewSize * sizeof yynewItems[0]);
  if (! yynewItems)
    yyMemoryExhausted (yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize;
       0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
    {
      *yyp1 = *yyp0;
      if (*(yybool *) yyp0)
        {
          yyGLRState* yys0 = &yyp0->yystate;
          yyGLRState* yys1 = &yyp1->yystate;
          if (yys0->yypred != YY_NULLPTR)
            yys1->yypred =
              YYRELOC (yyp0, yyp1, yys0->yypred, yystate);
          if (! yys0->yyresolved && yys0->yysemantics.yyfirstVal != YY_NULLPTR)
            yys1->yysemantics.yyfirstVal =
              YYRELOC (yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
        }
      else
        {
          yySemanticOption* yyv0 = &yyp0->yyoption;
          yySemanticOption* yyv1 = &yyp1->yyoption;
          if (yyv0->yystate != YY_NULLPTR)
            yyv1->yystate = YYRELOC (yyp0, yyp1, yyv0->yystate, yystate);
          if (yyv0->yynext != YY_NULLPTR)
            yyv1->yynext = YYRELOC (yyp0, yyp1, yyv0->yynext, yyoption);
        }
    }
  if (yystackp->yysplitPoint != YY_NULLPTR)
    yystackp->yysplitPoint = YYRELOC (yystackp->yyitems, yynewItems,
                                      yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != YY_NULLPTR)
      yystackp->yytops.yystates[yyn] =
        YYRELOC (yystackp->yyitems, yynewItems,
                 yystackp->yytops.yystates[yyn], yystate);
  YYFREE (yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void
yyfreeGLRStack (yyGLRStack* yystackp)
{
  YYFREE (yystackp->yyitems);
  yyfreeStateSet (&yystackp->yytops);
}

/** Assuming that YYS is a GLRState somewhere on *YYSTACKP, update the
 *  splitpoint of *YYSTACKP, if needed, so that it is at least as deep as
 *  YYS.  */
static inline void
yyupdateSplit (yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != YY_NULLPTR && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #YYK in *YYSTACKP.  */
static inline void
yymarkStackDeleted (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = YY_NULLPTR;
}

/** Undelete the last stack in *YYSTACKP that was marked as deleted.  Can
    only be done once after a deletion, and only when all other stacks have
    been deleted.  */
static void
yyundeleteLastStack (yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == YY_NULLPTR || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YYDPRINTF ((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULLPTR;
}

static inline void
yyremoveDeletes (yyGLRStack* yystackp)
{
  size_t yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
    {
      if (yystackp->yytops.yystates[yyi] == YY_NULLPTR)
        {
          if (yyi == yyj)
            {
              YYDPRINTF ((stderr, "Removing dead stacks.\n"));
            }
          yystackp->yytops.yysize -= 1;
        }
      else
        {
          yystackp->yytops.yystates[yyj] = yystackp->yytops.yystates[yyi];
          /* In the current implementation, it's unnecessary to copy
             yystackp->yytops.yylookaheadNeeds[yyi] since, after
             yyremoveDeletes returns, the parser immediately either enters
             deterministic operation or shifts a token.  However, it doesn't
             hurt, and the code might evolve to need it.  */
          yystackp->yytops.yylookaheadNeeds[yyj] =
            yystackp->yytops.yylookaheadNeeds[yyi];
          if (yyj != yyi)
            {
              YYDPRINTF ((stderr, "Rename stack %lu -> %lu.\n",
                          (unsigned long int) yyi, (unsigned long int) yyj));
            }
          yyj += 1;
        }
      yyi += 1;
    }
}

/** Shift to a new state on stack #YYK of *YYSTACKP, corresponding to LR
 * state YYLRSTATE, at input position YYPOSN, with (resolved) semantic
 * value *YYVALP and source location *YYLOCP.  */
static inline void
yyglrShift (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
            size_t yyposn,
            YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yysval = *yyvalp;
  yynewState->yyloc = *yylocp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK (yystackp);
}

/** Shift stack #YYK of *YYSTACKP, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static inline void
yyglrShiftDefer (yyGLRStack* yystackp, size_t yyk, yyStateNum yylrState,
                 size_t yyposn, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;
  YYASSERT (yynewState->yyisState);

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULLPTR;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction (yystackp, yyk, yynewState, yyrhs, yyrule);
}

#if !YYDEBUG
# define YY_REDUCE_PRINT(Args)
#else
# define YY_REDUCE_PRINT(Args)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print Args;               \
} while (0)

/*----------------------------------------------------------------------.
| Report that stack #YYK of *YYSTACKP is going to be reduced by YYRULE. |
`----------------------------------------------------------------------*/

static inline void
yy_reduce_print (int yynormal, yyGLRStackItem* yyvsp, size_t yyk,
                 yyRuleNum yyrule, void * scanner, nix::ParseData * data)
{
  int yynrhs = yyrhsLength (yyrule);
  int yylow = 1;
  int yyi;
  YYFPRINTF (stderr, "Reducing stack %lu by rule %d (line %lu):\n",
             (unsigned long int) yyk, yyrule - 1,
             (unsigned long int) yyrline[yyrule]);
  if (! yynormal)
    yyfillin (yyvsp, 1, -yynrhs);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyvsp[yyi - yynrhs + 1].yystate.yylrState],
                       &yyvsp[yyi - yynrhs + 1].yystate.yysemantics.yysval
                       , &(((yyGLRStackItem const *)yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yyloc)                       , scanner, data);
      if (!yyvsp[yyi - yynrhs + 1].yystate.yyresolved)
        YYFPRINTF (stderr, " (unresolved)");
      YYFPRINTF (stderr, "\n");
    }
}
#endif

/** Pop the symbols consumed by reduction #YYRULE from the top of stack
 *  #YYK of *YYSTACKP, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *YYVALP to the resulting value,
 *  and *YYLOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static inline YYRESULTTAG
yydoAction (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
            YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  int yynrhs = yyrhsLength (yyrule);

  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      /* Standard special case: single stack.  */
      yyGLRStackItem* yyrhs = (yyGLRStackItem*) yystackp->yytops.yystates[yyk];
      YYASSERT (yyk == 0);
      yystackp->yynextFree -= yynrhs;
      yystackp->yyspaceLeft += yynrhs;
      yystackp->yytops.yystates[0] = & yystackp->yynextFree[-1].yystate;
      YY_REDUCE_PRINT ((1, yyrhs, yyk, yyrule, scanner, data));
      return yyuserAction (yyrule, yynrhs, yyrhs, yystackp,
                           yyvalp, yylocp, scanner, data);
    }
  else
    {
      int yyi;
      yyGLRState* yys;
      yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
      yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred
        = yystackp->yytops.yystates[yyk];
      if (yynrhs == 0)
        /* Set default location.  */
        yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yys->yyloc;
      for (yyi = 0; yyi < yynrhs; yyi += 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yystackp->yytops.yystates[yyk] = yys;
      YY_REDUCE_PRINT ((0, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1, yyk, yyrule, scanner, data));
      return yyuserAction (yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp, yylocp, scanner, data);
    }
}

/** Pop items off stack #YYK of *YYSTACKP according to grammar rule YYRULE,
 *  and push back on the resulting nonterminal symbol.  Perform the
 *  semantic action associated with YYRULE and store its value with the
 *  newly pushed state, if YYFORCEEVAL or if *YYSTACKP is currently
 *  unambiguous.  Otherwise, store the deferred semantic action with
 *  the new state.  If the new state would have an identical input
 *  position, LR state, and predecessor to an existing state on the stack,
 *  it is identified with that existing state, eliminating stack #YYK from
 *  *YYSTACKP.  In this case, the semantic value is
 *  added to the options for the existing state's semantic value.
 */
static inline YYRESULTTAG
yyglrReduce (yyGLRStack* yystackp, size_t yyk, yyRuleNum yyrule,
             yybool yyforceEval, void * scanner, nix::ParseData * data)
{
  size_t yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULLPTR)
    {
      YYSTYPE yysval;
      YYLTYPE yyloc;

      YYRESULTTAG yyflag = yydoAction (yystackp, yyk, yyrule, &yysval, &yyloc, scanner, data);
      if (yyflag == yyerr && yystackp->yysplitPoint != YY_NULLPTR)
        {
          YYDPRINTF ((stderr, "Parse on stack %lu rejected by rule #%d.\n",
                     (unsigned long int) yyk, yyrule - 1));
        }
      if (yyflag != yyok)
        return yyflag;
      YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyrule], &yysval, &yyloc);
      yyglrShift (yystackp, yyk,
                  yyLRgotoState (yystackp->yytops.yystates[yyk]->yylrState,
                                 yylhsNonterm (yyrule)),
                  yyposn, &yysval, &yyloc);
    }
  else
    {
      size_t yyi;
      int yyn;
      yyGLRState* yys, *yys0 = yystackp->yytops.yystates[yyk];
      yyStateNum yynewLRState;

      for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength (yyrule);
           0 < yyn; yyn -= 1)
        {
          yys = yys->yypred;
          YYASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yynewLRState = yyLRgotoState (yys->yylrState, yylhsNonterm (yyrule));
      YYDPRINTF ((stderr,
                  "Reduced stack %lu by rule #%d; action deferred.  "
                  "Now in state %d.\n",
                  (unsigned long int) yyk, yyrule - 1, yynewLRState));
      for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
        if (yyi != yyk && yystackp->yytops.yystates[yyi] != YY_NULLPTR)
          {
            yyGLRState *yysplit = yystackp->yysplitPoint;
            yyGLRState *yyp = yystackp->yytops.yystates[yyi];
            while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
              {
                if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
                  {
                    yyaddDeferredAction (yystackp, yyk, yyp, yys0, yyrule);
                    yymarkStackDeleted (yystackp, yyk);
                    YYDPRINTF ((stderr, "Merging stack %lu into stack %lu.\n",
                                (unsigned long int) yyk,
                                (unsigned long int) yyi));
                    return yyok;
                  }
                yyp = yyp->yypred;
              }
          }
      yystackp->yytops.yystates[yyk] = yys;
      yyglrShiftDefer (yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
    }
  return yyok;
}

static size_t
yysplitStack (yyGLRStack* yystackp, size_t yyk)
{
  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      YYASSERT (yyk == 0);
      yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
    }
  if (yystackp->yytops.yysize >= yystackp->yytops.yycapacity)
    {
      yyGLRState** yynewStates;
      yybool* yynewLookaheadNeeds;

      yynewStates = YY_NULLPTR;

      if (yystackp->yytops.yycapacity
          > (YYSIZEMAX / (2 * sizeof yynewStates[0])))
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yycapacity *= 2;

      yynewStates =
        (yyGLRState**) YYREALLOC (yystackp->yytops.yystates,
                                  (yystackp->yytops.yycapacity
                                   * sizeof yynewStates[0]));
      if (yynewStates == YY_NULLPTR)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yystates = yynewStates;

      yynewLookaheadNeeds =
        (yybool*) YYREALLOC (yystackp->yytops.yylookaheadNeeds,
                             (yystackp->yytops.yycapacity
                              * sizeof yynewLookaheadNeeds[0]));
      if (yynewLookaheadNeeds == YY_NULLPTR)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
    }
  yystackp->yytops.yystates[yystackp->yytops.yysize]
    = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize]
    = yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize-1;
}

/** True iff YYY0 and YYY1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool
yyidenticalOptions (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
    {
      yyGLRState *yys0, *yys1;
      int yyn;
      for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
           yyn = yyrhsLength (yyy0->yyrule);
           yyn > 0;
           yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
        if (yys0->yyposn != yys1->yyposn)
          return yyfalse;
      return yytrue;
    }
  else
    return yyfalse;
}

/** Assuming identicalOptions (YYY0,YYY1), destructively merge the
 *  alternative semantic values for the RHS-symbols of YYY1 and YYY0.  */
static void
yymergeOptionSets (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
       yyn = yyrhsLength (yyy0->yyrule);
       yyn > 0;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
    {
      if (yys0 == yys1)
        break;
      else if (yys0->yyresolved)
        {
          yys1->yyresolved = yytrue;
          yys1->yysemantics.yysval = yys0->yysemantics.yysval;
        }
      else if (yys1->yyresolved)
        {
          yys0->yyresolved = yytrue;
          yys0->yysemantics.yysval = yys1->yysemantics.yysval;
        }
      else
        {
          yySemanticOption** yyz0p = &yys0->yysemantics.yyfirstVal;
          yySemanticOption* yyz1 = yys1->yysemantics.yyfirstVal;
          while (yytrue)
            {
              if (yyz1 == *yyz0p || yyz1 == YY_NULLPTR)
                break;
              else if (*yyz0p == YY_NULLPTR)
                {
                  *yyz0p = yyz1;
                  break;
                }
              else if (*yyz0p < yyz1)
                {
                  yySemanticOption* yyz = *yyz0p;
                  *yyz0p = yyz1;
                  yyz1 = yyz1->yynext;
                  (*yyz0p)->yynext = yyz;
                }
              yyz0p = &(*yyz0p)->yynext;
            }
          yys1->yysemantics.yyfirstVal = yys0->yysemantics.yyfirstVal;
        }
    }
}

/** Y0 and Y1 represent two possible actions to take in a given
 *  parsing state; return 0 if no combination is possible,
 *  1 if user-mergeable, 2 if Y0 is preferred, 3 if Y1 is preferred.  */
static int
yypreference (yySemanticOption* y0, yySemanticOption* y1)
{
  yyRuleNum r0 = y0->yyrule, r1 = y1->yyrule;
  int p0 = yydprec[r0], p1 = yydprec[r1];

  if (p0 == p1)
    {
      if (yymerger[r0] == 0 || yymerger[r0] != yymerger[r1])
        return 0;
      else
        return 1;
    }
  if (p0 == 0 || p1 == 0)
    return 0;
  if (p0 < p1)
    return 3;
  if (p1 < p0)
    return 2;
  return 0;
}

static YYRESULTTAG yyresolveValue (yyGLRState* yys,
                                   yyGLRStack* yystackp, void * scanner, nix::ParseData * data);


/** Resolve the previous YYN states starting at and including state YYS
 *  on *YYSTACKP. If result != yyok, some states may have been left
 *  unresolved possibly with empty semantic option chains.  Regardless
 *  of whether result = yyok, each state has been left with consistent
 *  data so that yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveStates (yyGLRState* yys, int yyn,
                 yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (0 < yyn)
    {
      YYASSERT (yys->yypred);
      YYCHK (yyresolveStates (yys->yypred, yyn-1, yystackp, scanner, data));
      if (! yys->yyresolved)
        YYCHK (yyresolveValue (yys, yystackp, scanner, data));
    }
  return yyok;
}

/** Resolve the states for the RHS of YYOPT on *YYSTACKP, perform its
 *  user action, and return the semantic value and location in *YYVALP
 *  and *YYLOCP.  Regardless of whether result = yyok, all RHS states
 *  have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG
yyresolveAction (yySemanticOption* yyopt, yyGLRStack* yystackp,
                 YYSTYPE* yyvalp, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs = yyrhsLength (yyopt->yyrule);
  YYRESULTTAG yyflag =
    yyresolveStates (yyopt->yystate, yynrhs, yystackp, scanner, data);
  if (yyflag != yyok)
    {
      yyGLRState *yys;
      for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
        yydestroyGLRState ("Cleanup: popping", yys, scanner, data);
      return yyflag;
    }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  if (yynrhs == 0)
    /* Set default location.  */
    yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yyopt->yystate->yyloc;
  {
    int yychar_current = yychar;
    YYSTYPE yylval_current = yylval;
    YYLTYPE yylloc_current = yylloc;
    yychar = yyopt->yyrawchar;
    yylval = yyopt->yyval;
    yylloc = yyopt->yyloc;
    yyflag = yyuserAction (yyopt->yyrule, yynrhs,
                           yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyvalp, yylocp, scanner, data);
    yychar = yychar_current;
    yylval = yylval_current;
    yylloc = yylloc_current;
  }
  return yyflag;
}

#if YYDEBUG
static void
yyreportTree (yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength (yyx->yyrule);
  int yyi;
  yyGLRState* yys;
  yyGLRState* yystates[1 + YYMAXRHS];
  yyGLRState yyleftmost_state;

  for (yyi = yynrhs, yys = yyx->yystate; 0 < yyi; yyi -= 1, yys = yys->yypred)
    yystates[yyi] = yys;
  if (yys == YY_NULLPTR)
    {
      yyleftmost_state.yyposn = 0;
      yystates[0] = &yyleftmost_state;
    }
  else
    yystates[0] = yys;

  if (yyx->yystate->yyposn < yys->yyposn + 1)
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, empty>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1);
  else
    YYFPRINTF (stderr, "%*s%s -> <Rule %d, tokens %lu .. %lu>\n",
               yyindent, "", yytokenName (yylhsNonterm (yyx->yyrule)),
               yyx->yyrule - 1, (unsigned long int) (yys->yyposn + 1),
               (unsigned long int) yyx->yystate->yyposn);
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
    {
      if (yystates[yyi]->yyresolved)
        {
          if (yystates[yyi-1]->yyposn+1 > yystates[yyi]->yyposn)
            YYFPRINTF (stderr, "%*s%s <empty>\n", yyindent+2, "",
                       yytokenName (yystos[yystates[yyi]->yylrState]));
          else
            YYFPRINTF (stderr, "%*s%s <tokens %lu .. %lu>\n", yyindent+2, "",
                       yytokenName (yystos[yystates[yyi]->yylrState]),
                       (unsigned long int) (yystates[yyi-1]->yyposn + 1),
                       (unsigned long int) yystates[yyi]->yyposn);
        }
      else
        yyreportTree (yystates[yyi]->yysemantics.yyfirstVal, yyindent+2);
    }
}
#endif

static YYRESULTTAG
yyreportAmbiguity (yySemanticOption* yyx0,
                   yySemanticOption* yyx1, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  YYUSE (yyx0);
  YYUSE (yyx1);

#if YYDEBUG
  YYFPRINTF (stderr, "Ambiguity detected.\n");
  YYFPRINTF (stderr, "Option 1,\n");
  yyreportTree (yyx0, 2);
  YYFPRINTF (stderr, "\nOption 2,\n");
  yyreportTree (yyx1, 2);
  YYFPRINTF (stderr, "\n");
#endif

  yyerror (yylocp, scanner, data, YY_("syntax is ambiguous"));
  return yyabort;
}

/** Resolve the locations for each of the YYN1 states in *YYSTACKP,
 *  ending at YYS1.  Has no effect on previously resolved states.
 *  The first semantic option of a state is always chosen.  */
static void
yyresolveLocations (yyGLRState* yys1, int yyn1,
                    yyGLRStack *yystackp, void * scanner, nix::ParseData * data)
{
  if (0 < yyn1)
    {
      yyresolveLocations (yys1->yypred, yyn1 - 1, yystackp, scanner, data);
      if (!yys1->yyresolved)
        {
          yyGLRStackItem yyrhsloc[1 + YYMAXRHS];
          int yynrhs;
          yySemanticOption *yyoption = yys1->yysemantics.yyfirstVal;
          YYASSERT (yyoption != YY_NULLPTR);
          yynrhs = yyrhsLength (yyoption->yyrule);
          if (yynrhs > 0)
            {
              yyGLRState *yys;
              int yyn;
              yyresolveLocations (yyoption->yystate, yynrhs,
                                  yystackp, scanner, data);
              for (yys = yyoption->yystate, yyn = yynrhs;
                   yyn > 0;
                   yys = yys->yypred, yyn -= 1)
                yyrhsloc[yyn].yystate.yyloc = yys->yyloc;
            }
          else
            {
              /* Both yyresolveAction and yyresolveLocations traverse the GSS
                 in reverse rightmost order.  It is only necessary to invoke
                 yyresolveLocations on a subforest for which yyresolveAction
                 would have been invoked next had an ambiguity not been
                 detected.  Thus the location of the previous state (but not
                 necessarily the previous state itself) is guaranteed to be
                 resolved already.  */
              yyGLRState *yyprevious = yyoption->yystate;
              yyrhsloc[0].yystate.yyloc = yyprevious->yyloc;
            }
          {
            int yychar_current = yychar;
            YYSTYPE yylval_current = yylval;
            YYLTYPE yylloc_current = yylloc;
            yychar = yyoption->yyrawchar;
            yylval = yyoption->yyval;
            yylloc = yyoption->yyloc;
            YYLLOC_DEFAULT ((yys1->yyloc), yyrhsloc, yynrhs);
            yychar = yychar_current;
            yylval = yylval_current;
            yylloc = yylloc_current;
          }
        }
    }
}

/** Resolve the ambiguity represented in state YYS in *YYSTACKP,
 *  perform the indicated actions, and set the semantic value of YYS.
 *  If result != yyok, the chain of semantic options in YYS has been
 *  cleared instead or it has been left unmodified except that
 *  redundant options may have been removed.  Regardless of whether
 *  result = yyok, YYS has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest = yyoptionList;
  yySemanticOption** yypp;
  yybool yymerge = yyfalse;
  YYSTYPE yysval;
  YYRESULTTAG yyflag;
  YYLTYPE *yylocp = &yys->yyloc;

  for (yypp = &yyoptionList->yynext; *yypp != YY_NULLPTR; )
    {
      yySemanticOption* yyp = *yypp;

      if (yyidenticalOptions (yybest, yyp))
        {
          yymergeOptionSets (yybest, yyp);
          *yypp = yyp->yynext;
        }
      else
        {
          switch (yypreference (yybest, yyp))
            {
            case 0:
              yyresolveLocations (yys, 1, yystackp, scanner, data);
              return yyreportAmbiguity (yybest, yyp, yylocp, scanner, data);
              break;
            case 1:
              yymerge = yytrue;
              break;
            case 2:
              break;
            case 3:
              yybest = yyp;
              yymerge = yyfalse;
              break;
            default:
              /* This cannot happen so it is not worth a YYASSERT (yyfalse),
                 but some compilers complain if the default case is
                 omitted.  */
              break;
            }
          yypp = &yyp->yynext;
        }
    }

  if (yymerge)
    {
      yySemanticOption* yyp;
      int yyprec = yydprec[yybest->yyrule];
      yyflag = yyresolveAction (yybest, yystackp, &yysval, yylocp, scanner, data);
      if (yyflag == yyok)
        for (yyp = yybest->yynext; yyp != YY_NULLPTR; yyp = yyp->yynext)
          {
            if (yyprec == yydprec[yyp->yyrule])
              {
                YYSTYPE yysval_other;
                YYLTYPE yydummy;
                yyflag = yyresolveAction (yyp, yystackp, &yysval_other, &yydummy, scanner, data);
                if (yyflag != yyok)
                  {
                    yydestruct ("Cleanup: discarding incompletely merged value for",
                                yystos[yys->yylrState],
                                &yysval, yylocp, scanner, data);
                    break;
                  }
                yyuserMerge (yymerger[yyp->yyrule], &yysval, &yysval_other);
              }
          }
    }
  else
    yyflag = yyresolveAction (yybest, yystackp, &yysval, yylocp, scanner, data);

  if (yyflag == yyok)
    {
      yys->yyresolved = yytrue;
      yys->yysemantics.yysval = yysval;
    }
  else
    yys->yysemantics.yyfirstVal = YY_NULLPTR;
  return yyflag;
}

static YYRESULTTAG
yyresolveStack (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (yystackp->yysplitPoint != YY_NULLPTR)
    {
      yyGLRState* yys;
      int yyn;

      for (yyn = 0, yys = yystackp->yytops.yystates[0];
           yys != yystackp->yysplitPoint;
           yys = yys->yypred, yyn += 1)
        continue;
      YYCHK (yyresolveStates (yystackp->yytops.yystates[0], yyn, yystackp
                             , scanner, data));
    }
  return yyok;
}

static void
yycompressStack (yyGLRStack* yystackp)
{
  yyGLRState* yyp, *yyq, *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULLPTR)
    return;

  for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULLPTR;
       yyp != yystackp->yysplitPoint;
       yyr = yyp, yyp = yyq, yyq = yyp->yypred)
    yyp->yypred = yyr;

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = ((yyGLRStackItem*) yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= yystackp->yynextFree - yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;

  while (yyr != YY_NULLPTR)
    {
      yystackp->yynextFree->yystate = *yyr;
      yyr = yyr->yypred;
      yystackp->yynextFree->yystate.yypred = &yystackp->yynextFree[-1].yystate;
      yystackp->yytops.yystates[0] = &yystackp->yynextFree->yystate;
      yystackp->yynextFree += 1;
      yystackp->yyspaceLeft -= 1;
    }
}

static YYRESULTTAG
yyprocessOneStack (yyGLRStack* yystackp, size_t yyk,
                   size_t yyposn, YYLTYPE *yylocp, void * scanner, nix::ParseData * data)
{
  while (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    {
      yyStateNum yystate = yystackp->yytops.yystates[yyk]->yylrState;
      YYDPRINTF ((stderr, "Stack %lu Entering state %d\n",
                  (unsigned long int) yyk, yystate));

      YYASSERT (yystate != YYFINAL);

      if (yyisDefaultedState (yystate))
        {
          YYRESULTTAG yyflag;
          yyRuleNum yyrule = yydefaultAction (yystate);
          if (yyrule == 0)
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          yyflag = yyglrReduce (yystackp, yyk, yyrule, yyimmediate[yyrule], scanner, data);
          if (yyflag == yyerr)
            {
              YYDPRINTF ((stderr,
                          "Stack %lu dies "
                          "(predicate failure or explicit user error).\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          if (yyflag != yyok)
            return yyflag;
        }
      else
        {
          yySymbol yytoken;
          int yyaction;
          const short int* yyconflicts;

          yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;
          if (yychar == YYEMPTY)
            {
              YYDPRINTF ((stderr, "Reading a token: "));
              yychar = yylex (&yylval, &yylloc, scanner, data);
            }

          if (yychar <= YYEOF)
            {
              yychar = yytoken = YYEOF;
              YYDPRINTF ((stderr, "Now at end of input.\n"));
            }
          else
            {
              yytoken = YYTRANSLATE (yychar);
              YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
            }

          yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);

          while (*yyconflicts != 0)
            {
              YYRESULTTAG yyflag;
              size_t yynewStack = yysplitStack (yystackp, yyk);
              YYDPRINTF ((stderr, "Splitting off stack %lu from %lu.\n",
                          (unsigned long int) yynewStack,
                          (unsigned long int) yyk));
              yyflag = yyglrReduce (yystackp, yynewStack,
                                    *yyconflicts,
                                    yyimmediate[*yyconflicts], scanner, data);
              if (yyflag == yyok)
                YYCHK (yyprocessOneStack (yystackp, yynewStack,
                                          yyposn, yylocp, scanner, data));
              else if (yyflag == yyerr)
                {
                  YYDPRINTF ((stderr, "Stack %lu dies.\n",
                              (unsigned long int) yynewStack));
                  yymarkStackDeleted (yystackp, yynewStack);
                }
              else
                return yyflag;
              yyconflicts += 1;
            }

          if (yyisShiftAction (yyaction))
            break;
          else if (yyisErrorAction (yyaction))
            {
              YYDPRINTF ((stderr, "Stack %lu dies.\n",
                          (unsigned long int) yyk));
              yymarkStackDeleted (yystackp, yyk);
              break;
            }
          else
            {
              YYRESULTTAG yyflag = yyglrReduce (yystackp, yyk, -yyaction,
                                                yyimmediate[-yyaction], scanner, data);
              if (yyflag == yyerr)
                {
                  YYDPRINTF ((stderr,
                              "Stack %lu dies "
                              "(predicate failure or explicit user error).\n",
                              (unsigned long int) yyk));
                  yymarkStackDeleted (yystackp, yyk);
                  break;
                }
              else if (yyflag != yyok)
                return yyflag;
            }
        }
    }
  return yyok;
}

static void
yyreportSyntaxError (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  if (yystackp->yyerrState != 0)
    return;
#if ! YYERROR_VERBOSE
  yyerror (&yylloc, scanner, data, YY_("syntax error"));
#else
  {
  yySymbol yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);
  size_t yysize0 = yytnamerr (YY_NULLPTR, yytokenName (yytoken));
  size_t yysize = yysize0;
  yybool yysize_overflow = yyfalse;
  char* yymsg = YY_NULLPTR;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected").  */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[yystackp->yytops.yystates[0]->yylrState];
      yyarg[yycount++] = yytokenName (yytoken);
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for this
             state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;
          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytokenName (yyx);
                {
                  size_t yysz = yysize + yytnamerr (YY_NULLPTR, yytokenName (yyx));
                  yysize_overflow |= yysz < yysize;
                  yysize = yysz;
                }
              }
        }
    }

  switch (yycount)
    {
#define YYCASE_(N, S)                   \
      case N:                           \
        yyformat = S;                   \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  {
    size_t yysz = yysize + strlen (yyformat);
    yysize_overflow |= yysz < yysize;
    yysize = yysz;
  }

  if (!yysize_overflow)
    yymsg = (char *) YYMALLOC (yysize);

  if (yymsg)
    {
      char *yyp = yymsg;
      int yyi = 0;
      while ((*yyp = *yyformat))
        {
          if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
            {
              yyp += yytnamerr (yyp, yyarg[yyi++]);
              yyformat += 2;
            }
          else
            {
              yyp++;
              yyformat++;
            }
        }
      yyerror (&yylloc, scanner, data, yymsg);
      YYFREE (yymsg);
    }
  else
    {
      yyerror (&yylloc, scanner, data, YY_("syntax error"));
      yyMemoryExhausted (yystackp);
    }
  }
#endif /* YYERROR_VERBOSE */
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
static void
yyrecoverSyntaxError (yyGLRStack* yystackp, void * scanner, nix::ParseData * data)
{
  size_t yyk;
  int yyj;

  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (yytrue)
      {
        yySymbol yytoken;
        if (yychar == YYEOF)
          yyFail (yystackp, &yylloc, scanner, data, YY_NULLPTR);
        if (yychar != YYEMPTY)
          {
            /* We throw away the lookahead, but the error range
               of the shifted error token must take it into account.  */
            yyGLRState *yys = yystackp->yytops.yystates[0];
            yyGLRStackItem yyerror_range[3];
            yyerror_range[1].yystate.yyloc = yys->yyloc;
            yyerror_range[2].yystate.yyloc = yylloc;
            YYLLOC_DEFAULT ((yys->yyloc), yyerror_range, 2);
            yytoken = YYTRANSLATE (yychar);
            yydestruct ("Error: discarding",
                        yytoken, &yylval, &yylloc, scanner, data);
          }
        YYDPRINTF ((stderr, "Reading a token: "));
        yychar = yylex (&yylval, &yylloc, scanner, data);
        if (yychar <= YYEOF)
          {
            yychar = yytoken = YYEOF;
            YYDPRINTF ((stderr, "Now at end of input.\n"));
          }
        else
          {
            yytoken = YYTRANSLATE (yychar);
            YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
          }
        yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
        if (yypact_value_is_default (yyj))
          return;
        yyj += yytoken;
        if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
          {
            if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
              return;
          }
        else if (! yytable_value_is_error (yytable[yyj]))
          return;
      }

  /* Reduce to one stack.  */
  for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
    if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
      break;
  if (yyk >= yystackp->yytops.yysize)
    yyFail (yystackp, &yylloc, scanner, data, YY_NULLPTR);
  for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
    yymarkStackDeleted (yystackp, yyk);
  yyremoveDeletes (yystackp);
  yycompressStack (yystackp);

  /* Now pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULLPTR)
    {
      yyGLRState *yys = yystackp->yytops.yystates[0];
      yyj = yypact[yys->yylrState];
      if (! yypact_value_is_default (yyj))
        {
          yyj += YYTERROR;
          if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYTERROR
              && yyisShiftAction (yytable[yyj]))
            {
              /* Shift the error token.  */
              /* First adjust its location.*/
              YYLTYPE yyerrloc;
              yystackp->yyerror_range[2].yystate.yyloc = yylloc;
              YYLLOC_DEFAULT (yyerrloc, (yystackp->yyerror_range), 2);
              YY_SYMBOL_PRINT ("Shifting", yystos[yytable[yyj]],
                               &yylval, &yyerrloc);
              yyglrShift (yystackp, 0, yytable[yyj],
                          yys->yyposn, &yylval, &yyerrloc);
              yys = yystackp->yytops.yystates[0];
              break;
            }
        }
      yystackp->yyerror_range[1].yystate.yyloc = yys->yyloc;
      if (yys->yypred != YY_NULLPTR)
        yydestroyGLRState ("Error: popping", yys, scanner, data);
      yystackp->yytops.yystates[0] = yys->yypred;
      yystackp->yynextFree -= 1;
      yystackp->yyspaceLeft += 1;
    }
  if (yystackp->yytops.yystates[0] == YY_NULLPTR)
    yyFail (yystackp, &yylloc, scanner, data, YY_NULLPTR);
}

#define YYCHK1(YYE)                                                          \
  do {                                                                       \
    switch (YYE) {                                                           \
    case yyok:                                                               \
      break;                                                                 \
    case yyabort:                                                            \
      goto yyabortlab;                                                       \
    case yyaccept:                                                           \
      goto yyacceptlab;                                                      \
    case yyerr:                                                              \
      goto yyuser_error;                                                     \
    default:                                                                 \
      goto yybuglab;                                                         \
    }                                                                        \
  } while (0)

/*----------.
| yyparse.  |
`----------*/

int
yyparse (void * scanner, nix::ParseData * data)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  size_t yyposn;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY;
  yylval = yyval_default;
  yylloc = yyloc_default;

  if (! yyinitGLRStack (yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP (yystack.yyexception_buffer))
    {
    case 0: break;
    case 1: goto yyabortlab;
    case 2: goto yyexhaustedlab;
    default: goto yybuglab;
    }
  yyglrShift (&yystack, 0, 0, 0, &yylval, &yylloc);
  yyposn = 0;

  while (yytrue)
    {
      /* For efficiency, we have two loops, the first of which is
         specialized to deterministic operation (single stack, no
         potential ambiguity).  */
      /* Standard mode */
      while (yytrue)
        {
          yyRuleNum yyrule;
          int yyaction;
          const short int* yyconflicts;

          yyStateNum yystate = yystack.yytops.yystates[0]->yylrState;
          YYDPRINTF ((stderr, "Entering state %d\n", yystate));
          if (yystate == YYFINAL)
            goto yyacceptlab;
          if (yyisDefaultedState (yystate))
            {
              yyrule = yydefaultAction (yystate);
              if (yyrule == 0)
                {
               yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  yyreportSyntaxError (&yystack, scanner, data);
                  goto yyuser_error;
                }
              YYCHK1 (yyglrReduce (&yystack, 0, yyrule, yytrue, scanner, data));
            }
          else
            {
              yySymbol yytoken;
              if (yychar == YYEMPTY)
                {
                  YYDPRINTF ((stderr, "Reading a token: "));
                  yychar = yylex (&yylval, &yylloc, scanner, data);
                }

              if (yychar <= YYEOF)
                {
                  yychar = yytoken = YYEOF;
                  YYDPRINTF ((stderr, "Now at end of input.\n"));
                }
              else
                {
                  yytoken = YYTRANSLATE (yychar);
                  YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
                }

              yygetLRActions (yystate, yytoken, &yyaction, &yyconflicts);
              if (*yyconflicts != 0)
                break;
              if (yyisShiftAction (yyaction))
                {
                  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
                  yychar = YYEMPTY;
                  yyposn += 1;
                  yyglrShift (&yystack, 0, yyaction, yyposn, &yylval, &yylloc);
                  if (0 < yystack.yyerrState)
                    yystack.yyerrState -= 1;
                }
              else if (yyisErrorAction (yyaction))
                {
               yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  yyreportSyntaxError (&yystack, scanner, data);
                  goto yyuser_error;
                }
              else
                YYCHK1 (yyglrReduce (&yystack, 0, -yyaction, yytrue, scanner, data));
            }
        }

      while (yytrue)
        {
          yySymbol yytoken_to_shift;
          size_t yys;

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            yystackp->yytops.yylookaheadNeeds[yys] = yychar != YYEMPTY;

          /* yyprocessOneStack returns one of three things:

              - An error flag.  If the caller is yyprocessOneStack, it
                immediately returns as well.  When the caller is finally
                yyparse, it jumps to an error label via YYCHK1.

              - yyok, but yyprocessOneStack has invoked yymarkStackDeleted
                (&yystack, yys), which sets the top state of yys to NULL.  Thus,
                yyparse's following invocation of yyremoveDeletes will remove
                the stack.

              - yyok, when ready to shift a token.

             Except in the first case, yyparse will invoke yyremoveDeletes and
             then shift the next token onto all remaining stacks.  This
             synchronization of the shift (that is, after all preceding
             reductions on all stacks) helps prevent double destructor calls
             on yylval in the event of memory exhaustion.  */

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            YYCHK1 (yyprocessOneStack (&yystack, yys, yyposn, &yylloc, scanner, data));
          yyremoveDeletes (&yystack);
          if (yystack.yytops.yysize == 0)
            {
              yyundeleteLastStack (&yystack);
              if (yystack.yytops.yysize == 0)
                yyFail (&yystack, &yylloc, scanner, data, YY_("syntax error"));
              YYCHK1 (yyresolveStack (&yystack, scanner, data));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
           yystack.yyerror_range[1].yystate.yyloc = yylloc;
              yyreportSyntaxError (&yystack, scanner, data);
              goto yyuser_error;
            }

          /* If any yyglrShift call fails, it will fail after shifting.  Thus,
             a copy of yylval will already be on stack 0 in the event of a
             failure in the following loop.  Thus, yychar is set to YYEMPTY
             before the loop to make sure the user destructor for yylval isn't
             called twice.  */
          yytoken_to_shift = YYTRANSLATE (yychar);
          yychar = YYEMPTY;
          yyposn += 1;
          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            {
              int yyaction;
              const short int* yyconflicts;
              yyStateNum yystate = yystack.yytops.yystates[yys]->yylrState;
              yygetLRActions (yystate, yytoken_to_shift, &yyaction,
                              &yyconflicts);
              /* Note that yyconflicts were handled by yyprocessOneStack.  */
              YYDPRINTF ((stderr, "On stack %lu, ", (unsigned long int) yys));
              YY_SYMBOL_PRINT ("shifting", yytoken_to_shift, &yylval, &yylloc);
              yyglrShift (&yystack, yys, yyaction, yyposn,
                          &yylval, &yylloc);
              YYDPRINTF ((stderr, "Stack %lu now in state #%d\n",
                          (unsigned long int) yys,
                          yystack.yytops.yystates[yys]->yylrState));
            }

          if (yystack.yytops.yysize == 1)
            {
              YYCHK1 (yyresolveStack (&yystack, scanner, data));
              YYDPRINTF ((stderr, "Returning to deterministic operation.\n"));
              yycompressStack (&yystack);
              break;
            }
        }
      continue;
    yyuser_error:
      yyrecoverSyntaxError (&yystack, scanner, data);
      yyposn = yystack.yytops.yystates[0]->yyposn;
    }

 yyacceptlab:
  yyresult = 0;
  goto yyreturn;

 yybuglab:
  YYASSERT (yyfalse);
  goto yyabortlab;

 yyabortlab:
  yyresult = 1;
  goto yyreturn;

 yyexhaustedlab:
  yyerror (&yylloc, scanner, data, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;

 yyreturn:
  if (yychar != YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead",
                YYTRANSLATE (yychar), &yylval, &yylloc, scanner, data);

  /* If the stack is well-formed, pop the stack until it is empty,
     destroying its entries as we go.  But free the stack regardless
     of whether it is well-formed.  */
  if (yystack.yyitems)
    {
      yyGLRState** yystates = yystack.yytops.yystates;
      if (yystates)
        {
          size_t yysize = yystack.yytops.yysize;
          size_t yyk;
          for (yyk = 0; yyk < yysize; yyk += 1)
            if (yystates[yyk])
              {
                while (yystates[yyk])
                  {
                    yyGLRState *yys = yystates[yyk];
                 yystack.yyerror_range[1].yystate.yyloc = yys->yyloc;
                  if (yys->yypred != YY_NULLPTR)
                      yydestroyGLRState ("Cleanup: popping", yys, scanner, data);
                    yystates[yyk] = yys->yypred;
                    yystack.yynextFree -= 1;
                    yystack.yyspaceLeft += 1;
                  }
                break;
              }
        }
      yyfreeGLRStack (&yystack);
    }

  return yyresult;
}

/* DEBUGGING ONLY */
#if YYDEBUG
static void
yy_yypstack (yyGLRState* yys)
{
  if (yys->yypred)
    {
      yy_yypstack (yys->yypred);
      YYFPRINTF (stderr, " -> ");
    }
  YYFPRINTF (stderr, "%d@%lu", yys->yylrState,
             (unsigned long int) yys->yyposn);
}

static void
yypstates (yyGLRState* yyst)
{
  if (yyst == YY_NULLPTR)
    YYFPRINTF (stderr, "<null>");
  else
    yy_yypstack (yyst);
  YYFPRINTF (stderr, "\n");
}

static void
yypstack (yyGLRStack* yystackp, size_t yyk)
{
  yypstates (yystackp->yytops.yystates[yyk]);
}

#define YYINDEX(YYX)                                                         \
    ((YYX) == YY_NULLPTR ? -1 : (yyGLRStackItem*) (YYX) - yystackp->yyitems)


static void
yypdumpstack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yyp;
  size_t yyi;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
    {
      YYFPRINTF (stderr, "%3lu. ",
                 (unsigned long int) (yyp - yystackp->yyitems));
      if (*(yybool *) yyp)
        {
          YYASSERT (yyp->yystate.yyisState);
          YYASSERT (yyp->yyoption.yyisState);
          YYFPRINTF (stderr, "Res: %d, LR State: %d, posn: %lu, pred: %ld",
                     yyp->yystate.yyresolved, yyp->yystate.yylrState,
                     (unsigned long int) yyp->yystate.yyposn,
                     (long int) YYINDEX (yyp->yystate.yypred));
          if (! yyp->yystate.yyresolved)
            YYFPRINTF (stderr, ", firstVal: %ld",
                       (long int) YYINDEX (yyp->yystate
                                             .yysemantics.yyfirstVal));
        }
      else
        {
          YYASSERT (!yyp->yystate.yyisState);
          YYASSERT (!yyp->yyoption.yyisState);
          YYFPRINTF (stderr, "Option. rule: %d, state: %ld, next: %ld",
                     yyp->yyoption.yyrule - 1,
                     (long int) YYINDEX (yyp->yyoption.yystate),
                     (long int) YYINDEX (yyp->yyoption.yynext));
        }
      YYFPRINTF (stderr, "\n");
    }
  YYFPRINTF (stderr, "Tops:");
  for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
    YYFPRINTF (stderr, "%lu: %ld; ", (unsigned long int) yyi,
               (long int) YYINDEX (yystackp->yytops.yystates[yyi]));
  YYFPRINTF (stderr, "\n");
}
#endif

#undef yylval
#undef yychar
#undef yynerrs
#undef yylloc



#line 510 "src/libexpr/parser.y" /* glr.c:2584  */



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <eval.hh>
#include <download.hh>
#include <store-api.hh>


namespace nix {


Expr * EvalState::parse(const char * text,
    const Path & path, const Path & basePath, StaticEnv & staticEnv)
{
    yyscan_t scanner;
    ParseData data(*this);
    data.basePath = basePath;
    data.path = data.symbols.create(path);

    yylex_init(&scanner);
    yy_scan_string(text, scanner);
    int res = yyparse(scanner, &data);
    yylex_destroy(scanner);

    if (res) throw ParseError(data.error);

    data.result->bindVars(staticEnv);

    return data.result;
}


Path resolveExprPath(Path path)
{
    assert(path[0] == '/');

    /* If `path' is a symlink, follow it.  This is so that relative
       path references work. */
    struct stat st;
    while (true) {
        if (lstat(path.c_str(), &st))
            throw SysError(format("getting status of ‘%1%’") % path);
        if (!S_ISLNK(st.st_mode)) break;
        path = absPath(readLink(path), dirOf(path));
    }

    /* If `path' refers to a directory, append `/default.nix'. */
    if (S_ISDIR(st.st_mode))
        path = canonPath(path + "/default.nix");

    return path;
}


Expr * EvalState::parseExprFromFile(const Path & path)
{
    return parseExprFromFile(path, staticBaseEnv);
}


Expr * EvalState::parseExprFromFile(const Path & path, StaticEnv & staticEnv)
{
    return parse(readFile(path).c_str(), path, dirOf(path), staticEnv);
}


Expr * EvalState::parseExprFromString(const string & s, const Path & basePath, StaticEnv & staticEnv)
{
    return parse(s.c_str(), "(string)", basePath, staticEnv);
}


Expr * EvalState::parseExprFromString(const string & s, const Path & basePath)
{
    return parseExprFromString(s, basePath, staticBaseEnv);
}


void EvalState::addToSearchPath(const string & s)
{
    size_t pos = s.find('=');
    string prefix;
    Path path;
    if (pos == string::npos) {
        path = s;
    } else {
        prefix = string(s, 0, pos);
        path = string(s, pos + 1);
    }

    searchPath.emplace_back(prefix, path);
}


Path EvalState::findFile(const string & path)
{
    return findFile(searchPath, path);
}


Path EvalState::findFile(SearchPath & searchPath, const string & path, const Pos & pos)
{
    for (auto & i : searchPath) {
        std::string suffix;
        if (i.first.empty())
            suffix = "/" + path;
        else {
            auto s = i.first.size();
            if (path.compare(0, s, i.first) != 0 ||
                (path.size() > s && path[s] != '/'))
                continue;
            suffix = path.size() == s ? "" : "/" + string(path, s);
        }
        auto r = resolveSearchPathElem(i);
        if (!r.first) continue;
        Path res = r.second + suffix;
        if (pathExists(res)) return canonPath(res);
    }
    format f = format(
        "file ‘%1%’ was not found in the Nix search path (add it using $NIX_PATH or -I)"
        + string(pos ? ", at %2%" : ""));
    f.exceptions(boost::io::all_error_bits ^ boost::io::too_many_args_bit);
    throw ThrownError(f % path % pos);
}


std::pair<bool, std::string> EvalState::resolveSearchPathElem(const SearchPathElem & elem)
{
    auto i = searchPathResolved.find(elem.second);
    if (i != searchPathResolved.end()) return i->second;

    std::pair<bool, std::string> res;

    if (isUri(elem.second)) {
        try {
            res = { true, downloadFileCached(elem.second, true) };
        } catch (DownloadError & e) {
            printMsg(lvlError, format("warning: Nix search path entry ‘%1%’ cannot be downloaded, ignoring") % elem.second);
            res = { false, "" };
        }
    } else {
        auto path = absPath(elem.second);
        if (pathExists(path))
            res = { true, path };
        else {
            printMsg(lvlError, format("warning: Nix search path entry ‘%1%’ does not exist, ignoring") % elem.second);
            res = { false, "" };
        }
    }

    debug(format("resolved search path element ‘%s’ to ‘%s’") % elem.second % res.second);

    searchPathResolved[elem.second] = res;
    return res;
}


}
