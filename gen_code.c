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
    bof_write_magic_to_header(&ret);
    ret.text_start_address = 0;
    // remember, the unit of length in the BOF format is a byte!
    ret.text_length = code_seq_size(main_cs);
    int dsa = MAX(ret.text_length, 1024);
    ret.data_start_address = dsa;
    ret.data_length = literal_table_size();
    int sba = dsa + ret.data_length + STACK_SPACE;
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
    
    
    // Start with constant variable declarations
    code_seq_concat(&main_cs, gen_code_const_decls(prog.const_decls));
    code_seq_concat(&main_cs, gen_code_var_decls(prog.var_decls));
    int vars_length = (code_seq_size(main_cs) / 2);
    code_seq_concat(&main_cs, code_utils_save_registers_for_AR());

    // Then do statements (assign, call, if, while, read, print, block)
    code_seq_concat(&main_cs, gen_code_stmts(prog.stmts));
    code_seq_concat(&main_cs, code_utils_restore_registers_from_AR());
    code_seq_concat(&main_cs, code_utils_deallocate_stack_space(vars_length));

    code_seq_concat(&main_cs, code_utils_tear_down_program());
    gen_code_output_program(bf, main_cs);
}

code_seq gen_code_var_decls(var_decls_t vds) {
    code_seq ret = code_seq_empty();

    var_decl_t* var_decl = vds.var_decls;
    while(var_decl != NULL) {
        code_seq_concat(&ret, gen_code_var_decl(*var_decl));
        var_decl = var_decl->next;
    }

    return ret;
}

code_seq gen_code_var_decl(var_decl_t vd) {
    return gen_code_idents(vd.ident_list);
}

code_seq gen_code_const_decls(const_decls_t cds) {
    code_seq ret = code_seq_empty();

    const_decl_t* const_decl = cds.start;
    while(const_decl != NULL) {
        code_seq_concat(&ret, gen_code_const_decl(*const_decl));
        const_decl = const_decl->next;
    }

    return ret;
}

code_seq gen_code_const_decl(const_decl_t cd) {
    return gen_code_const_defs(cd.const_def_list);
}

code_seq gen_code_const_defs(const_def_list_t cds) {
    code_seq ret = code_seq_empty();

    const_def_t* cd = cds.start;
    while(cd != NULL) {
        code_seq_concat(&ret, gen_code_const_def(*cd));
        cd = cd->next;
    }

    return ret;
}

code_seq gen_code_const_def(const_def_t cd) {
    code_seq ret = code_seq_empty();

    code_seq_add_to_end(&ret, code_sri(SP, 1));

    code_seq_concat(&ret, gen_code_number(cd.number));

    int offset = id_use_get_attrs(cd.ident.idu)->offset_count;
    code_seq_add_to_end(&ret, code_swr(GP, offset, SP));

    code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
    code_seq_add_to_end(&ret, code_ari(SP, 1));

    return ret;
}

code_seq gen_code_idents(ident_list_t idents) {
    code_seq ret = code_seq_empty();

    code_seq_add_to_end(&ret, code_sri(SP, 1));

    ident_t *ident = idents.start;
    while(ident != NULL){
        int offset = id_use_get_attrs(ident->idu)->offset_count;
        code_seq_add_to_end(&ret, code_swr(GP, offset, SP));
        ident = ident->next;
    }

    code_seq_add_to_end(&ret, code_ari(SP, 1));
    return ret;
}

// Generate code for the list of statments given by stmts to out
code_seq gen_code_stmts(stmts_t stmts) {
    code_seq ret = code_seq_empty();

    if(stmts.stmts_kind != empty_stmts_e) {
        stmt_t* stmt = stmts.stmt_list.start;
        while(stmt != NULL) {
            code_seq_concat(&ret, gen_code_stmt(*stmt));
            stmt = stmt->next;
        }
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
    code_seq ret = code_seq_empty();

    code_seq_add_to_end(&ret, code_sri(SP, 1));
    code_seq_concat(&ret, gen_code_expr(*stmt.expr));

    // Evaluation of expr is now at top of stack (SP).
    assert(stmt.idu != NULL);
    assert(id_use_get_attrs(stmt.idu) != NULL);
    id_kind kind = id_use_get_attrs(stmt.idu)->kind;

    switch(kind) {
        case constant_idk: {
            bail_with_error("No assignment for constant");
            return code_seq_empty();
        }
        case variable_idk: {
            code_seq_concat(&ret, code_utils_compute_fp(GP, stmt.idu->levelsOutward));
            int offset = id_use_get_attrs(stmt.idu)->offset_count;
            assert(offset < USHRT_MAX);
            code_seq_add_to_end(&ret, code_swr(GP, offset, SP));
        }
        case procedure_idk: {
            bail_with_error("No implementation for procedures");
            return code_seq_empty();
        }
        default: {
            bail_with_error("Invalid id_kind");
            return code_seq_empty();
        }
    }


    code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
    code_seq_add_to_end(&ret, code_ari(SP, 1));

    return ret;
}

code_seq gen_code_call_stmt(call_stmt_t stmt) {
    bail_with_error("TODO: no implementation of gen_code_call_stmt yet!");
    return code_seq_empty();
}

code_seq gen_code_if_stmt(if_stmt_t stmt) {
    code_seq ret = code_seq_empty();

    // SP = 1, and SP+1 should have the evaluation of the condition stored in it. (0 for false, 1 for true)

    code_seq_concat(&ret, gen_code_condition(stmt.condition));

    code_seq then_body = gen_code_stmts(*(stmt.then_stmts));
    code_seq else_body = code_seq_empty();
    if(stmt.else_stmts->stmts_kind != empty_stmts_e) {
        else_body = gen_code_stmts(*(stmt.else_stmts));

        // Adding instruction to jump over else stmts at end of then statements.
        code_seq_add_to_end(&then_body, code_beq(SP, 0, code_seq_size(else_body)+1));
    } 

    // Adding instruction to jump over then stmts. This instruction should be skipped if the condition evaluates to true.
    code_seq_add_to_end(&ret, code_beq(SP, 0, code_seq_size(then_body)+1));

    // Adding then stmts
    code_seq_concat(&ret, then_body);

    // Adding else stmts (adds nothing if else is empty)
    code_seq_concat(&ret, else_body);

    return ret;
}

code_seq gen_code_condition(condition_t cond) {
    switch(cond.cond_kind) {
        case ck_db: {
            return gen_code_db_condition(cond.data.db_cond);
        }
        case ck_rel: {
            return gen_code_rel_op_condition(cond.data.rel_op_cond);
        }
        default: {
            bail_with_error("Invalid cond_kind. Code: %d", cond.cond_kind);
            return code_seq_empty();
        }
    }
}

code_seq gen_code_while_stmt(while_stmt_t stmt) {
    code_seq ret = code_seq_empty();

    code_seq while_body = gen_code_stmts(*stmt.body);

    // Adding instruction to jump over the while body.
    code_seq_add_to_end(&ret, code_beq(SP, 0, code_seq_size(while_body)+1));

    // Adding while body instructions (initially skipped)
    code_seq_concat(&ret, while_body);

    // Adding condition instructions after while body
    code_seq_concat(&ret, gen_code_condition(stmt.condition));

    // Adding jump instruction to go back to beginning of while statement. This is skipped if condition is false.
    code_seq_add_to_end(&ret, code_jrel(-code_seq_size(ret)));
    
    return ret;
}

code_seq gen_code_read_stmt(read_stmt_t stmt) {
    const char *name = stmt.name;
    if (!name){
        bail_with_error("There is no name");
        return code_seq_empty();
    }

    return code_seq_singleton(code_rch(GP, literal_table_lookup(name, stmt.idu->levelsOutward)));
}

code_seq gen_code_print_stmt(print_stmt_t stmt) {
    expr_t expr = stmt.expr;
    switch(expr.expr_kind) {
        case expr_number: {
            number_t num = expr.data.number;
            return code_seq_singleton(code_pint(GP, literal_table_lookup(num.text, num.value)));
        }
        default: {
            bail_with_error("Invalid expr_kind. Code: %d", expr.expr_kind);
            return code_seq_empty();
        }
    }
}

code_seq gen_code_block_stmt(block_stmt_t stmt) {
    // Start with constant and variable declarations
    code_seq ret = gen_code_const_decls(stmt.block->const_decls);
    code_seq_concat(&ret,  gen_code_var_decls(stmt.block->var_decls));
    int vars_length = (code_seq_size(ret) / 2);
    code_seq_concat(&ret, code_utils_save_registers_for_AR());

    // Then do statements (assign, call, if, while, read, print, block)
    code_seq_concat(&ret, gen_code_stmts(stmt.block->stmts));
    code_seq_concat(&ret, code_utils_restore_registers_from_AR());
    code_seq_concat(&ret, code_utils_deallocate_stack_space(vars_length));

    code_seq_concat(&ret, code_utils_tear_down_program());

    return ret;
}

code_seq gen_code_rel_op_condition(rel_op_condition_t cond) {
    code_seq ret = code_seq_empty();

    // Adding a space to the stack for expr2
    code_seq_add_to_end(&ret, code_sri(SP, 1)); 

    // Evaluating expr2 and putting its value at top of stack
    code_seq_concat(&ret, gen_code_expr(cond.expr2));

    // Adding a space to the stack for expr1
    code_seq_add_to_end(&ret, code_sri(SP, 1)); 

    // Evaluating expr1 and putting its value at top of stack
    code_seq_concat(&ret, gen_code_expr(cond.expr1));

    // top of stack (SP) should be expr1, and expr2 should be next.

    switch(cond.rel_op.code) {
        case eqeqsym: {
            // [SP] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // If SP == 0, then expr1 must have been equal to expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_beq(SP, 0, 2));
            break;
        }
        case neqsym: {
            // [SP] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // If SP != 0, then expr1 must have not been equal to expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_bne(SP, 0, 2));
            break;
        }
        case ltsym: {
            // [SP+2] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // Top of stack should now have value of expr1 - expr2

            // If SP < 0, then expr1 must have been less than expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_bltz(SP, 0, 2));
            // If SP > 0, then expr1 must have been greater than expr2, which is false. So it doesn't skip the instruction that skips the then stmts.

            break;
        }
        case leqsym: {
            // [SP] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // If SP <= 0, then expr1 must have been less than or equal to expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_blez(SP, 0, 2));
            break;
        }
        case gtsym: {
            // [SP] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // If SP > 0, then expr1 must have been greater than expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_bgtz(SP, 0, 2));
            break;
        }
        case geqsym: {
            // [SP] = expr1 - expr2
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));

            // Removing both spaces from top of stack.
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));
            code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
            code_seq_add_to_end(&ret, code_ari(SP, 1));

            // If SP >= 0, then expr1 must have been greater than or equal to expr2, which is true. So it skips the instruction that skips the then stmts.
            code_seq_add_to_end(&ret, code_bgez(SP, 0, 2));
            break;
        }
        default: {
            bail_with_error("Invalid opcode. Code: %d", cond.rel_op.code);
            return ret;
        }
    }    

    return ret;
}

code_seq gen_code_db_condition(db_condition_t cond) {
    bail_with_error("TODO: no implementation of gen_code_db_condition yet!");
    return code_seq_empty();
}

code_seq gen_code_expr(expr_t expr) {
    switch(expr.expr_kind) {
        case expr_bin: {
            return gen_code_binary_op_expr(expr.data.binary);
        }
        case expr_negated: {
            bail_with_error("No implementation for expr_negated");
            return code_seq_empty();
        }
        case expr_ident: {
            return gen_code_ident(expr.data.ident);
        }
        case expr_number: {
            return gen_code_number(expr.data.number);
        }
        default: {
            bail_with_error("Invalid expr_kind. Code: %d", expr.expr_kind);
            return code_seq_empty();
        }
    }
}

code_seq gen_code_binary_op_expr(binary_op_expr_t bin) {
    code_seq ret = code_seq_empty();
    
    // Adding a space to the stack for expr2
    code_seq_add_to_end(&ret, code_sri(SP, 1));

    // Evaluating expr2 and putting its value at top of stack
    code_seq_concat(&ret, gen_code_expr(*bin.expr2));

    // Adding a space to the stack for expr1
    code_seq_add_to_end(&ret, code_sri(SP, 1));

    // Evaluating expr1 and putting its value at top of stack
    code_seq_concat(&ret, gen_code_expr(*bin.expr1));

    // top of stack (SP) should be expr1, and expr2 should be next.

    switch(bin.arith_op.code) {
        case plussym: {
            code_seq_add_to_end(&ret, code_add(SP, 2, SP, 1));
            break;
        }
        case minussym: {
            code_seq_add_to_end(&ret, code_sub(SP, 2, SP, 1));
            break; 
        }
        case multsym: {
            code_seq_add_to_end(&ret, code_mul(SP, 1));
            break;
        }
        case divsym: {
            code_seq_add_to_end(&ret, code_div(SP, 1));
            break;
        }
        default: {
            bail_with_error("Invalid opcode. Code: %d", bin.arith_op.code);
            return code_seq_empty();
        }
    }
    code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
    code_seq_add_to_end(&ret, code_ari(SP, 1));
    code_seq_add_to_end(&ret, code_sub(SP, 0, SP, 0));
    code_seq_add_to_end(&ret, code_ari(SP, 1));

    return ret;
}

// Putting the value referenced by ident at the top of the stack
code_seq gen_code_ident(ident_t ident) {
    assert(ident.idu != NULL);

    code_seq ret = code_utils_compute_fp(SP, ident.idu->levelsOutward);

    assert(id_use_get_attrs(ident.idu) != NULL);

    int offset = id_use_get_attrs(ident.idu)->offset_count;

    assert(offset < USHRT_MAX); // making sure it fits

    code_seq_add_to_end(&ret, code_lwr(SP, GP, offset));

    return ret;
}

// Putting the value stored in num at the top of the stack
code_seq gen_code_number(number_t num) {
    return code_seq_singleton(code_addi(SP, 0, num.value));
}