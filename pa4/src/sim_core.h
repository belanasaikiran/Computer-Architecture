/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 * 
 * Programming Assignment 3: MultiCycle Stall + In order + Seperate Execution Units
 * 
 * riscy-uconn: instruction_map.h
 * 
 * DO NOT MODIFY THIS FILE
 * 
 */

#pragma once

#include <stdio.h>
#include <stdint.h>

extern FILE *fptr_pt;

/* Max number of registers, and instruction length in bits */
#define MAX_LENGTH 32

/* Array of registers (register file) */
extern int registers[MAX_LENGTH];

/* Clock cycle */
extern int cycle;

/* Program Counter (PC) register */
extern unsigned int pc;     // Current PC
extern unsigned int pc_n;   // Next PC

/* Microarchitechtual state */
extern struct State fetch_out, fetch_out_n;
extern struct State decode_out, decode_out_n;
extern struct State ex_out, ex_out_n;
extern struct State ex_ld_st_out, ex_ld_st_out_n;
extern struct State ex_ld_st_2_out, ex_ld_st_2_out_n;
extern struct State wb_out, wb_out_n;
extern struct State wb_ld_st_out, wb_ld_st_out_n;
extern struct State wb_ld_st_2_out, wb_ld_st_2_out_n;


/* nop instruction, used when flushing the pipeline */
extern const struct State nop;

/* Instruction and data memory */
extern int *memory;

/* Instruction and cycle counters */
extern int instruction_counter;
extern int cycle;

/* CPU state */
struct State {
     /* Fetched instruction */
     unsigned int inst;
     unsigned int inst_addr;

     /* Decoded instruction fields */
     unsigned int opcode;
     unsigned int funct3;
     unsigned int funct7;
     unsigned int rd;
     unsigned int rs1;
     unsigned int rs2;
     unsigned int imm;

     /* Memory related */
     unsigned int mem_buffer;
     unsigned int mem_addr;

     /* Branch Related */
     unsigned int br_addr;
     unsigned int link_addr;

     /* ALU */
     unsigned int alu_in1;
     unsigned int alu_in2;
     unsigned int alu_out;

     // LD/ST unit reservation
     unsigned int ld_st_unit; // 0 for none, 1 for unit 1, 2 for unit 2 ...
};


/* Pipeline related */
extern int forwarding_enabled;
extern int ooo_enabled;
extern int pipe_stall;
extern int j_taken;
extern int br_mispredicted;
extern int we_exe, ws_exe, dout_exe;
extern int we_mem, ws_mem, dout_mem;
extern int we_mem2, ws_mem2, dout_mem2;
extern int we_wb,  ws_wb,  dout_wb;
extern int we_ld_st_wb,  ws_ld_st_wb,  dout_ld_st_wb;
extern int we_ld_st_2_wb,  ws_ld_st_2_wb,  dout_ld_st_2_wb;



/* Multi-cycle operation-related */
extern const int dmem_access_cycles;
extern int dmem_busy;
extern int dmem_cycles;
extern int dmem_busy2;
extern int dmem_cycles2;



void initialize(FILE *fp);
void process_instructions();