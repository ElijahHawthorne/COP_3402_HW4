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

    gen_code_output_program(bf, main_cs);
}