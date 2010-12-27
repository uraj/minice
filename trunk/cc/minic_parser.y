%{
/* this parser reads input from stdin */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <getopt.h>
#include "minic_typedef.h"
#include "minic_ast.h"
#include "minic_typetree.h"
#include "minic_symtable.h"
#include "minic_symfunc.h"
#include "minic_astcheck.h"
#include "minic_triargtable.h"
#include "minic_triargexpr.h"
#include "minic_basicblock.h"
#include "minic_flowanalyse.h"
#include "minic_aliasanalyse.h"
#include "minic_regalloc.h"
#include "minic_machinecode.h"
//#define DEBUG
//#define SHOWBNF
//#define SHOWLOCALCODE
//#define SHOWCODE

const size_t MAXIDLEN = 31; /* 31 is the minimum length supported by compilers according to ANCI C standard */

/* option switches */
int option_show_triargexpr = 0;
int option_show_localcode = 0;
int option_show_ast = 0;

/* provided by the scanner */
extern int yylineno;
extern int yylex();
extern FILE* yyin;
extern int yyerror(const char*);

struct symbol_table *simb_table;
struct symbol_table *curr_table;
struct symbol_stack *type_stack;
struct symbol_stack *parm_stack;
struct triargtable **table_list;/*all funcs' table in one array*/
int table_list_index;/*the current index to insert table*/
int table_list_bound;/*the capacity of the array current now*/

struct mach_code_table * code_table_list;

int g_table_list_size;/* size of the table list */
int g_global_id_num = 0;/* total global id num, increase when insert global id */

int g_const_str_num = 0;//total const string num, increase when insert const string

int g_var_id_num = 0;/* total id num, increase when insert id */
struct basic_block ** DFS_array;/* may only be a temp array */
int g_block_num;

struct triargexpr_list *ghead, *gtail;/*the tmp indirect table*/
struct triargexpr *gtriargexpr_table;/*the tmp direct table*/
int gtriargexpr_table_index;/*the current index of the direct table*/
int gtriargexpr_table_bound;/*the current capacity of the direct table*/

%}
%locations
%start program 
%union
{
    enum operator op_type;
    enum data_type dat_type;
    enum modifier modi_type;
    struct ast* tree;
    char id_str[31+1]; /* 31 is the maximum length of identifiers */
    struct value_type * v_type;
    struct token_info tk;
    struct taexpr_list_header * code;
}
%token <tk> IDENT ICONSTANT CCONSTANT SCONSTANT
%token VOID INT CHAR IF ELSE FOR WHILE RETURN EXTERN REGISTER
%token SEMICOLON        ";"
%token COMMA            ","
%token LEFTPARENTHESIS  "("
%token RIGHTPARENTHESIS ")"
%token LEFTBRACE        "{"
%token RIGHTBRACE       "}"
%token LEFTBRACKET      "["
%token RIGHTBRACKET     "]"
%token ASSIGN           "="
%token PLUS             "+"
%token MINUS            "-"
%token ASTERISK         "*"
%token AMPERSAND        "&"
%token NGE              "<"
%token NLE              ">"
%token LNOT             "!"
%token PLUSPLUS        "++"
%token MINUSMINUS      "--"
%token LAND            "&&"
%token LOR             "||"
%token NEQ             "!="
%token EQ              "=="
%token GE              ">="
%token LE              "<="

%type <tree> primary_expr postfix_expr unary_expr binary_expr assignment_expr expression constant argument_list
%type <dat_type> type_name
%type <modi_type> modifier
%type <v_type> scalar_var
%type <id_str> id function_hdr
%type <code> statement statement_list compound_stmt null_stmt expression_stmt if_stmt for_stmt while_stmt return_stmt

%left "||"
%left "&&"
%left "==" "!="
%left "<" ">" "<=" ">="
%left "+" "-"
%left "*"

/* to resolve conflicts in rule:
 * scalar_var -> IDENT | IDENT "(" parm_type_list ")"
 */
%nonassoc VAR
%nonassoc FUNC /* functions */

/* to resolve conflicts in rule:
 * if_stmt -> IF "(" expression ")" statement
 *          | IF "(" expression ")" statement ELSE statement
 */
%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE
%%

program : external_decls {
                             #ifdef SHOWBNF
                             printf("program -> external_decls\n");
                             #endif
                         }
        ;
        
external_decls : declaration external_decls {
												g_global_id_num = g_var_id_num;/* Record the global id num *//* MARK TAOTAOTHERIPPER */
                                                #ifdef SHOWBNF
                                                printf("external_decls -> declaration external_decls\n");
                                                #endif
                                            }
               | function_list {
                                   #ifdef SHOWBNF
                                   printf("external_decls -> function_list\n");
                                   #endif
                               }
               ;

declaration : modifier type_name var_list ";" {
                                                  #ifdef SHOWBNF
                                                  printf("declaration : modifier type_name var_list\n");
                                                  #endif
                                                  decl_retype_insert(type_stack, curr_table, $2, $1);
                                              }
            | type_name var_list ";" {
                                         #ifdef SHOWBNF
                                         printf("declaration : type_name var_list\n");
                                         #endif
                                         decl_retype_insert(type_stack, curr_table, $1, Epsilon);
                                     }
            ;

function_list : function_list function_def
              | function_def
              ;

modifier : EXTERN { $$ = Extern; }
         | REGISTER { $$ = Register; }
         ;
         
type_name : VOID { $$ = Void; }
          | INT { $$ = Int; }
          | CHAR { $$ = Char; }
          ;
          
var_list : var_list "," var_item
         | var_item /* original BNF wrong here */
         ;
         
var_item : array_var
         | scalar_var
         | "*" scalar_var {
                              decl_add_pointer($2);
                          }
         ;
         
array_var : id "[" ICONSTANT "]" {
                                     decl_push_array($1, yylval.tk.ival, type_stack);
                                 }
          ;

scalar_var: id %prec VAR {
                             $$ = decl_push_simple($1, type_stack);
                         }
          | id "(" parm_type_list ")" %prec FUNC {    
                                                     $$ = decl_push_func($1, type_stack, parm_stack);
                                                 }
          ;

function_def : function_hdr "{" function_body "}" {
                                                      curr_table = NULL;/*is not necessary*/
													  struct triargtable * tmp_table = new_table($1);
													  insert_table(tmp_table);/*add new function triargtable to the list,
																			   g_total_expr_num is modified in the function.*/
													  if(option_show_triargexpr)
													    print_table(tmp_table);
													  new_global_table();/*new a global table for next function*/	
                                                  }
             ;

function_hdr : type_name id "(" parm_type_list ")" {
                                                       curr_table = symt_new();
                                                       def_insert_func($2, parm_stack, simb_table,
                                                                       curr_table, $1, Simpletype);
													   strcpy($$, $2);/*send function name up*/
                                                   }
             | type_name "*" id "(" parm_type_list ")" {
                                                            curr_table = symt_new();
                                                            def_insert_func($3, parm_stack, simb_table, 
                                                                            curr_table, $1, Pointer);
															strcpy($$, $3);
                                                       }
             | id "(" parm_type_list ")" {
                                             curr_table = symt_new();
                                             def_insert_func($1, parm_stack, simb_table,
                                                             curr_table, Int, Simpletype);/*do as C do*/
											 strcpy($$, $1);
                                         }
             ;
             
parm_type_list : parm_list {
                               #ifdef SHOWBNF
                               printf("parm_type_list : parm_list\n");
                               #endif
                           }
               | VOID {
                          #ifdef SHOWBNF
                          printf("parm_type_list : VOID\n");
                          #endif
                      }
               | {
                     #ifdef SHOWBNF
                     printf("parm_type_list :\n");
                     #endif
                 }
               ;

parm_list : parm_list "," parm_decl {
                                        #ifdef SHOWBNF
                                        printf("parm_list : parm_list , parm_decl\n");
                                        #endif

                                    }
          | parm_decl {
                          #ifdef SHOWBNF
                          printf("parm_list : parm_decl\n");
                          #endif
                      }
          ;

parm_decl : type_name id {
                             #ifdef SHOWBNF
                             printf("parm_decl : type_name id\n");
                             #endif
                             dd_push_parm($2, parm_stack, $1, Simpletype);
                         }
          | type_name "*" id {
                                #ifdef SHOWBNF
                                printf("parm_decl : type_name * id\n");
                                #endif
                                dd_push_parm($3, parm_stack, $1, Pointer);
                             }
          ;

function_body : internal_decls statement_list {
                                                    
                                                   if(option_show_localcode)//#ifdef SHOWCODE
												        print_list_header($2);
													struct taexpr_list_header * final_list = stmt_list_merge($2, return_list_append(NULL));
                                                    
													ghead = final_list -> head;
                                                    gtail = final_list -> tail;
                                                    free_taexprlist(final_list);
                                              }
              ;

internal_decls : declaration internal_decls
               |
               ;

statement_list : statement statement_list { $$ = stmt_list_merge($1, $2); }
               | { $$ = NULL; }
               ;

statement : compound_stmt { $$ = $1;
                            #ifdef SHOWLOCALCODE
                                //print_list_header($$);//test
                            #endif
                          }
          | null_stmt { $$ = $1; }
          | expression_stmt { $$ = $1; }
          | if_stmt { $$ = $1; }
          | for_stmt { $$ = $1; }
          | while_stmt { $$ = $1; }
          | return_stmt { $$ = $1; }
          ;

compound_stmt : "{" statement_list "}" { $$ = $2; }
              ;

null_stmt : ";" { $$ = NULL; };

expression_stmt : expression ";" {
                                     if(option_show_ast)
                                        print_ast($1);
                                     ast_type_check($1, simb_table, curr_table);
                                     struct subexpr_info expr = triargexpr_gen($1);
                                     free_ast($1);
                                     $$ = expr_list_gen(&expr);
                                 }
                ;

if_stmt : IF "(" expression ")" statement %prec LOWER_THEN_ELSE {
                                                                    ast_type_check($3, simb_table, curr_table);
                                                                    if($3 -> ast_typetree -> type == Void)
                                                                    {
                                                                        fprintf(stderr, "Line %d: value of expression is void.\n", yylineno);
                                                                        exit(1); /* error report mechanism, policy needed. */
                                                                    }
                                                                    struct subexpr_info cond = triargexpr_gen($3);
                                                                    free_ast($3);
                                                                    $$ = if_list_merge(&cond, $5);
                                                                    if(option_show_localcode)
                                                                    {
																		printf("if:\n");
																		print_list_header($$);//test
																		printf("\n");
																    }
                                                                }
        | IF "(" expression ")" statement ELSE statement {
                                                             ast_type_check($3, simb_table, curr_table);
                                                             if($3 -> ast_typetree -> type == Void)
                                                             {
                                                                 fprintf(stderr, "Line %d: value of expression is void.\n", yylineno);
                                                                 exit(1); /* error report mechanism, policy needed. */
                                                             }
                                                             struct subexpr_info cond = triargexpr_gen($3);
                                                             free_ast($3);
                                                             $$ = if_else_list_merge(&cond, $5, $7);
                                                             if(option_show_localcode)
                                                             {
                                                                printf("if_else:\n");
																print_list_header($$);//test
																printf("\n");
                                                             }
                                                         }
        ;

for_stmt : FOR "(" expression ";" expression ";" expression ")" statement
                                                        {
                                                            ast_type_check($3, simb_table, curr_table);
                                                            struct subexpr_info init = triargexpr_gen($3);
                                                            free_ast($3);
                                                            ast_type_check($5, simb_table, curr_table);
                                                            struct subexpr_info cond = triargexpr_gen($5);
                                                            free_ast($5);
                                                            ast_type_check($7, simb_table, curr_table);
                                                            struct subexpr_info incr = triargexpr_gen($7);
                                                            free_ast($7);
                                                            $$ = for_list_merge(&init, &cond, &incr, $9);
                                                            if(option_show_localcode)
                                                            {
																printf("for:\n");
																print_list_header($$);//test
																printf("\n");
                                                            }
                                                        }
         ;

while_stmt : WHILE "(" expression ")" statement {
                                                    ast_type_check($3, simb_table, curr_table);
                                                    if($3 -> ast_typetree -> type == Void)
                                                    {
                                                        fprintf(stderr, "Line %d: value of expression is void.\n", yylineno);
                                                        exit(1); /* error report mechanism, policy needed. */
                                                    }
                                                    struct subexpr_info cond = triargexpr_gen($3);
                                                    free_ast($3);
                                                    $$ = while_list_merge(&cond, $5);



                                                    if(option_show_localcode)
                                                    {
														printf("while:\n");
                                                        print_list_header($$);//test
														printf("\n");
                                                    }
                                                }

           ;

return_stmt : RETURN expression ";" {
                                        $2 = new_ast(Return, 0, $2, NULL);
                                        ast_type_check($2, simb_table, curr_table);
                                        struct subexpr_info ret_value = triargexpr_gen($2);
										$$ = return_list_append(&ret_value);
                                    }
            | RETURN ";" {
                            $$ = return_list_append(NULL);
                         }
            ;

expression : assignment_expr
           ;

assignment_expr : unary_expr "=" expression { $$ = new_ast(Assign, 0, $1, $3); }
                | binary_expr /* default */
                ;

binary_expr : binary_expr "+" binary_expr  { $$ = new_ast(Plus, 0, $1, $3); }
            | binary_expr "-" binary_expr  { $$ = new_ast(Minus, 0, $1, $3); }
            | binary_expr "*" binary_expr  { $$ = new_ast(Mul, 0, $1, $3); }
            | binary_expr "<" binary_expr  { $$ = new_ast(Nge, 0, $1, $3); }
            | binary_expr ">" binary_expr  { $$ = new_ast(Nle, 0, $1, $3); }
            | binary_expr "==" binary_expr { $$ = new_ast(Eq, 0, $1, $3); }
            | binary_expr "!=" binary_expr { $$ = new_ast(Neq, 0, $1, $3); }
            | binary_expr ">=" binary_expr { $$ = new_ast(Ge, 0, $1, $3); }
            | binary_expr "<=" binary_expr { $$ = new_ast(Le, 0, $1, $3); }
            | binary_expr "&&" binary_expr { $$ = new_ast(Land, 0, $1, $3);}
            | binary_expr "||" binary_expr { $$ = new_ast(Lor, 0, $1, $3); }
            | unary_expr
            ;

unary_expr : "!" unary_expr {
                                $$ = new_ast(Lnot, 0, $2, NULL);
                                #ifdef SHOWBNF
                                printf("unary_expr : ! unary_expr\n");
                                #endif
                            }
           | "+" unary_expr {
                                $$ = new_ast(Uplus, 0, $2, NULL);
                                #ifdef SHOWBNF
                                printf("unary_expr : + unary_expr\n");
                                #endif
                            }
           | "-" unary_expr {
                                $$ = new_ast(Uminus, 0, $2, NULL);
                                #ifdef SHOWBNF
                                printf("unary_expr : - unary_expr\n");
                                #endif
                            }
           | "++" unary_expr {
                                 $$ = new_ast(Plusplus, 0, $2, NULL);
                                 #ifdef SHOWBNF
                                 printf("unary_expr : ++ unary_expr\n");
                                 #endif
                             }
           | "--" unary_expr {
                                 $$ = new_ast(Minusminus, 0, $2, NULL);
                                 #ifdef SHOWBNF
                                 printf("unary_expr : -- unary_expr\n");
                                 #endif
                             }
           | "&" unary_expr {
                                 $$ = new_ast(Ref, 0, $2, NULL);
                                 #ifdef SHOWBNF
                                 printf("unary_expr : & unary_expr\n");
                                 #endif
                            }
           | "*" unary_expr {
                                 $$ = new_ast(Deref, 0, $2, NULL);
                                 #ifdef SHOWBNF
                                 printf("unary_expr : * unary_expr\n");
                                 #endif
                            }
           | postfix_expr {
                              $$ = $1;
                              #ifdef SHOWBNF
                              printf("unary_expr : postfix_expr\n");
                              #endif
                          }
           | "&&" unary_expr {
                                 /* error recovery */
                                 $$ = NULL;
                                 fprintf(stderr, "Line %d: invalide expression: Land is a binary operator\n", yylineno);
                             }
           ;

postfix_expr : id "[" expression "]" {
                                         $$ = new_ast(Subscript, 0, new_ast(Nullop, 1, NULL, new_var($1)), $3);
                                         #ifdef SHOWBNF
                                         printf("postfix_expr : IDENT [ expression ]\n");
                                         #endif
                                     }
             | id "(" argument_list ")" {
                                            $$ = new_ast(Funcall, 0, new_ast(Nullop, 1, NULL, new_var($1)), $3);
                                            //print_ast($$);
                                            #ifdef SHOWBNF
                                            printf("postfix_expr : IDENT ( argument_list )\n");
                                            #endif
                                        }
             | postfix_expr "++" {
                                     $$ = new_ast(Plusplus, 0, $1, NULL);
                                     #ifdef SHOWBNF
                                     printf("postfix_expr : postfix_expr ++\n");
                                     #endif
                                 }
             | postfix_expr "--" {
                                     $$ = new_ast(Minusminus, 0, $1, NULL);
                                     #ifdef SHOWBNF
                                     printf("postfix_expr : postfix_expr --\n");
                                     #endif
                                 }
             | primary_expr {
                                $$ = $1;
                                #ifdef SHOWBNF
                                printf("postfix_expr : primary_expr\n");
                                #endif
                            }
             ;

primary_expr : id {
                         $$ = new_ast(Nullop, 1, NULL, new_var($1));
                         #ifdef SHOWBNF
                         printf("primary_expr : IDENT\n");
                         #endif
                  }
             | constant {
                            $$ = $1;
                            #ifdef SHOWBNF
                            printf("primary_expr : constant\n");
                            #endif
                        }
             | "(" expression ")" {
                                      $$ = $2;
                                      #ifdef SHOWBNF
                                      printf("primary_expr : ( expression )\n");
                                      #endif
                                  }
             ;

constant : ICONSTANT {
                         $$ = new_ast(Nullop, 1, NULL, new_iconst($1));
                         #ifdef SHOWBNF
                         printf("constant : ICONSTANT\n");
                         #endif
                     }
         | CCONSTANT {
                         $$ = new_ast(Nullop, 1, NULL, new_cconst($1));
                         #ifdef SHOWBNF
                         printf("constant : CCONSTANT\n");
                         #endif
                     }
         | SCONSTANT {
              struct token_info str_info = symt_insert_conststr(curr_table, strdup($1.sval));
              if(str_info.sval != NULL)
							$$ = new_ast(Nullop, 1, NULL, new_var(str_info.sval));
              
#ifdef SHOWBNF
                         printf("constant : SCONSTANT\n");
                         #endif
                     }
         ;

argument_list : expression "," argument_list {
                                                 $$ = new_ast(Arglist, 0, $1, $3);
                                                 #ifdef SHOWBNF
                                                 printf("argument_list : expression , argument_list\n");
                                                 #endif
                                             }
              | expression {
                               $$ = new_ast(Arglist, 0, $1, NULL);
                               #ifdef SHOWBNF
                               printf("argument_list : expression\n");
                               #endif
                           }
              | {
                    $$ = NULL;
                    #ifdef SHOWBNF
                    printf("arument_list :\n");
                    #endif
                }
              ;

id : IDENT {
               if(strlen($1.sval) > MAXIDLEN)
               {
                   fprintf(stderr, "Identifier \'%s\' too long at line:%d\n", $1.sval, yylineno);
                   exit(1);
               }
               strcpy($$, $1.sval);
               #ifdef DEBUG
               printf("ID %s at line %d\n", $$, yylineno);
               #endif
           }
%%
int main(int argc, char* argv[])
{
    char option = 0;
    char * filename = NULL;
    int i;
    for(i = 1; i < argc; ++i)
    {
        if(argv[i][0] != '-')
        {
            filename = argv[i];
            break;
        }
    }
    while((option = getopt(argc, argv, "amtf:")) != -1)
    {
        switch(option)
        {
            case 'a':
                option_show_ast = 1;
                break;
            case 't':
                option_show_triargexpr = 1;
                break;
            case 'm':
                option_show_localcode = 1;
                break;
            case 'f':
                filename = optarg;
                break;
            default:
                ;
        }
    }
    if(filename == NULL)
    {
        fprintf(stderr, "minicc: need a C source file as input\n");
        return 1;
    }
    else if((yyin = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "minicc: %s: No such file.\n", filename);
        return 1;
    }
	curr_table = symt_new();
    simb_table = curr_table;
    type_stack = syms_new();
    parm_stack = syms_new();
    new_table_list();/*new table list, bound and index has been setted in it*/
	new_global_table();/*new global table for current function*/
	yyparse();
	fclose(yyin);
	free_global_table();/*there should be an extra tmp table, and g_table_list_size is set in this*/
	new_code_table_list();
	print_file_header(stdout, filename);
	for(i = 0; i < g_table_list_size; i++)
	{
		//printf("%s\n", table_list[i] -> funcname);
		gen_machine_code(i, stdout);
		/*here is the register allotting and the assemble codes generating*/

	    /*
		int map_id_num = get_ref_var_num();
        struct ralloc_info alloc_reg = reg_alloc(curfun_actvar_lists , curfun_expr_num , map_id_num , 27);
        printf("\n");
        for(j = 0 ; j < map_id_num ; j++)
             printf("map_id%d is in regester%d\n" , j , alloc_reg.result[j]);
        printf("\ntotally consume %d regesters\n" , alloc_reg.consume);
        
        printf("varlist:\n");
        for(j = 0 ; j < curfun_expr_num ; j++)
        {
            var_list_print(curfun_actvar_lists + j);
            var_list_free_bynode(curfun_actvar_lists[j].head);
        }
        printf("varlist ends.\n");
		*/
	}
	print_file_tail(stdout);
	free_code_table_list();
    syms_delete(parm_stack);
    syms_delete(type_stack);
    symt_delete(curr_table);
    return 0;
}

int yyerror(const char* msg)
{
    fprintf(stderr, "%s\n", msg);
    return 0;
}
