/* $Id: gen_code.h,v 1.7 2023/11/14 04:41:00 leavens Exp $ */
#ifndef _GEN_CODE_H
#define _GEN_CODE_H
#include <stdio.h>
#include "ast.h"
#include "bof.h"
#include "instruction.h"
#include "code.h"
#include "code_seq.h"
#include <limits.h>
#include <string.h>
#include "ast.h"
#include "code.h"
#include "id_use.h"
#include "literal_table.h"
#include "utilities.h"
#include "regname.h"
#include "code_utils.h"
#include "spl.tab.h"
#include "symtab.h"

// Initialize the code generator
extern void gen_code_initialize();

// Requires: bf if open for writing in binary
// Generate code for the given AST
extern void gen_code_program(BOFFILE bf, block_t prog);

// Requires: bf if open for writing in binary
// Generate code for prog into bf
extern void gen_code_program(BOFFILE bf, block_t prog);

// Generate code for the var_decls_t vds to out
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
extern code_seq gen_code_var_decls(var_decls_t vds);

extern code_seq gen_code_const_decls(const_decls_t cds);

// Generate code for a single <var-decl>, vd,
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
extern code_seq gen_code_var_decl(var_decl_t vd);

extern code_seq gen_code_const_decl(const_decl_t cd);

// Generate code for the identififers in idents with type t
// in reverse order (so the first declared are allocated last).
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
extern code_seq gen_code_idents(ident_list_t idents);

extern code_seq gen_code_const_defs(const_def_list_t cds);

extern code_seq gen_code_const_def(const_def_t cd);

// Generate code for stmt
extern code_seq gen_code_stmt(stmt_t stmt);

// Generate code for stmt
extern code_seq gen_code_assign_stmt(assign_stmt_t stmt);

// Generate code for stmt
extern code_seq gen_code_block_stmt(block_stmt_t stmt);

// Generate code for the list of statments given by stmts to out
extern code_seq gen_code_stmts(stmts_t stmts);

// Generate code for the if-statment given by stmt
extern code_seq gen_code_if_stmt(if_stmt_t stmt);

extern code_seq gen_code_while_stmt(while_stmt_t stmt);

// Generate code for the call statement given by stmt
extern code_seq gen_code_call_stmt(call_stmt_t stmt);

// Generate code for the read statment given by stmt
extern code_seq gen_code_read_stmt(read_stmt_t stmt);

// Generate code for the write statment given by stmt.
extern code_seq gen_code_print_stmt(print_stmt_t stmt);

extern code_seq gen_code_condition(condition_t cond);

extern code_seq gen_code_rel_op_condition(rel_op_condition_t cond);

extern code_seq gen_code_db_condition(db_condition_t cond);

extern code_seq gen_code_expr(expr_t expr);

extern code_seq gen_code_binary_op_expr(binary_op_expr_t bin);

extern code_seq gen_code_ident(ident_t ident);

extern code_seq gen_code_number(number_t num);

extern code_seq gen_code_negated_expr(negated_expr_t expr);

#endif