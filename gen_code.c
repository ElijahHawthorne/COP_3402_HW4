/* $Id: gen_code.c,v 1.25 2023/11/28 22:12:58 leavens Exp $ */

#include "gen_code.h"

#define STACK_SPACE 4096

// Initialize the code generator
void gen_code_initialize()
{
    literal_table_initialize();
}

// Requires: bf if open for writing in binary
// and prior to this scope checking and type checking have been done.
// Write all the instructions in cs to bf in order
static void gen_code_output_seq(BOFFILE bf, code_seq cs)
{
    while (!code_seq_is_empty(cs)) {
	bin_instr_t inst = code_seq_first(cs)->instr;
	instruction_write_bin_instr(bf, inst);
	cs = code_seq_rest(cs);
    }
}

// Return a header appropriate for the give code
static BOFHeader gen_code_program_header(code_seq main_cs)
{
    BOFHeader ret;
    strncpy(ret.magic, "FBF", 4);  // for FLOAT SRM
    ret.text_start_address = 0;
    // remember, the unit of length in the BOF format is a byte!
    ret.text_length = code_seq_size(main_cs) * BYTES_PER_WORD;
    int dsa = MAX(ret.text_length, 1024);
    ret.data_start_address = dsa;
    ret.data_length = literal_table_size() * BYTES_PER_WORD;
    int sba = dsa + ret.data_length;
    ret.stack_bottom_addr = sba;
    return ret;
}

static void gen_code_output_literals(BOFFILE bf)
{
    literal_table_start_iteration();
    while (literal_table_iteration_has_next()) {
	word_type w = literal_table_iteration_next();
	// debug_print("Writing literal %f to BOF file\n", w);
	bof_write_word(bf, w);
    }
    literal_table_end_iteration(); // not necessary
}

// Requires: bf is open for writing in binary
// Write the program's BOFFILE to bf
static void gen_code_output_program(BOFFILE bf, code_seq main_cs)
{
    BOFHeader bfh = gen_code_program_header(main_cs);
    bof_write_header(bf, bfh);
    gen_code_output_seq(bf, main_cs);
    gen_code_output_literals(bf);
    bof_close(bf);
}


// Requires: bf if open for writing in binary
// Generate code for prog into bf
void gen_code_program(BOFFILE bf, block_t prog)
{
    code_seq main_cs = code_seq_empty();
    
    // Write here

    // Start with variable declarations

    // Then do statements (assign, call, if, while, read, print, block)
    code_seq_concat(&main_cs, gen_code_stmts(prog.stmts));

    gen_code_output_program(bf, main_cs);
}

// Generate code for the list of statments given by stmts to out
code_seq gen_code_stmts(stmts_t stmts) {
    stmt_t* stmt = stmts.stmt_list.start;

    code_seq ret = code_seq_empty();

    while(stmt != NULL) {
        code_seq_concat(&ret, gen_code_stmt(*stmt));
        stmt = stmt->next;
    }

    return ret;
}

code_seq gen_code_stmt(stmt_t stmt) {

    switch(stmt.stmt_kind) {
        case assign_stmt: {
            return gen_code_assign_stmt(stmt.data.assign_stmt);
        }
        case call_stmt: {
            return gen_code_call_stmt(stmt.data.call_stmt);
        }
        case if_stmt: {
            return gen_code_if_stmt(stmt.data.if_stmt);
        }
        case while_stmt: {
            return gen_code_while_stmt(stmt.data.while_stmt);
        }
        case read_stmt: {
            return gen_code_read_stmt(stmt.data.read_stmt);
        }
        case print_stmt: {
            return gen_code_print_stmt(stmt.data.print_stmt);
        }
        case block_stmt: {
            return gen_code_block_stmt(stmt.data.block_stmt);
        }
        default: {
            bail_with_error("Invalid stmt_kind");
            return code_seq_empty();
        }
    }
}

code_seq gen_code_assign_stmt(assign_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_assign_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_call_stmt(call_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_call_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_if_stmt(if_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_if_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_while_stmt(while_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_while_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_read_stmt(read_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_read_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_print_stmt(print_stmt_t stmt) {
    code_seq ret = gen_code_expr(stmt.expr);

    return ret;
}

code_seq gen_code_block_stmt(block_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_block_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_expr(expr_t expr) {
    switch(expr.expr_kind) {
        case expr_bin: {
            return gen_code_binary_op_expr(expr.data.binary);
        }
        case expr_negated: {
            return gen_code_logical_not_expr(expr.data.negated);
        }
        case expr_ident: {
            return gen_code_ident(expr.data.ident);
        }
        case expr_number: {
            return gen_code_number(expr.data.number);
        }
        default: {
            bail_with_error("Invalid expr_kind");
            return code_seq_empty();
        }
    }
}

code_seq gen_code_binary_op_expr(binary_op_expr_t expr) {
    bail_with_error("TODO: no implementation of gen_code_binary_op_expr yet!");
    return code_seq_empty();
}

code_seq gen_code_logical_not_expr(negated_expr_t expr) {
    bail_with_error("TODO: no implementation of gen_code_logical_not_expr yet!");
    return code_seq_empty();
}

code_seq gen_code_ident(ident_t id) {
    bail_with_error("TODO: no implementation of gen_code_ident yet!");
    return code_seq_empty();
}

code_seq gen_code_number(number_t num) {
    unsigned int offset = literal_table_lookup(num.text, num.value);
    return code_seq_singleton(code_pint(GP, offset));
}