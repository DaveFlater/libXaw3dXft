/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     OC = 258,
     CC = 259,
     OA = 260,
     CA = 261,
     OP = 262,
     CP = 263,
     NAME = 264,
     NUMBER = 265,
     INFINITY = 266,
     VERTICAL = 267,
     HORIZONTAL = 268,
     EQUAL = 269,
     DOLLAR = 270,
     MINUS = 271,
     PLUS = 272,
     PERCENTOF = 273,
     DIVIDE = 274,
     TIMES = 275,
     PERCENT = 276,
     HEIGHT = 277,
     WIDTH = 278,
     UPLUS = 279,
     UMINUS = 280
   };
#endif
/* Tokens.  */
#define OC 258
#define CC 259
#define OA 260
#define CA 261
#define OP 262
#define CP 263
#define NAME 264
#define NUMBER 265
#define INFINITY 266
#define VERTICAL 267
#define HORIZONTAL 268
#define EQUAL 269
#define DOLLAR 270
#define MINUS 271
#define PLUS 272
#define PERCENTOF 273
#define DIVIDE 274
#define TIMES 275
#define PERCENT 276
#define HEIGHT 277
#define WIDTH 278
#define UPLUS 279
#define UMINUS 280




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 22 "laygram.y"

    int		    ival;
    XrmQuark	    qval;
    BoxPtr	    bval;
    BoxParamsPtr    pval;
    GlueRec	    gval;
    LayoutDirection lval;
    ExprPtr	    eval;
    Operator	    oval;



/* Line 2068 of yacc.c  */
#line 113 "laygram.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE LayYYlval;


