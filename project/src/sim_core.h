/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 * 
 * Project: MultiCycle Stall + OOO + Seperate Execution Units + BP + Cache
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

/* Size of the BTB */
#define BTB_SIZE 32

/* Size of the BHSR (in bits) */
#define BHSR_SIZE 5

/* Size of the BHT */
#define BHT_SIZE 32


#define CACHE_SIZE 256 // bytes
#define CACHE_LINE_SIZE 16 // bytes

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
     unsigned int br_predicted; /* **NEW** from PA1 / PA2 */

     /* ALU */
     unsigned int alu_in1;
     unsigned int alu_in2;
     unsigned int alu_out;

     int cache_line_hit_way;

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
extern const int dcache_access_cycles;
extern int dmem_busy;
extern int dmem_cycles;
extern int dmem_busy2;
extern int dmem_cycles2;


/* BTB Stats-related */
extern int branch_prediction_enabled;
extern int total_branches;
extern int correctly_predicted_branches;

/* BHT States for two-bit prediction */
/* Encoding is N = '00', NT = '01', TN = '10', T = '11'*/
enum PREDICTION {N, NT, TN, T};

/* Structure of the branch predictor */
typedef struct {
   unsigned int inst_addr;
   unsigned int branch_target;
   unsigned int valid;
} BranchTargetBuffer;

/* Allocate memory for the BTB, BHT, and BHSR */
extern BranchTargetBuffer *btb;
extern enum PREDICTION *bht;
extern uint8_t bhsr;

/* Data Cache-related */
extern int dcache_enabled;
extern int dmem_accesses;
extern int dcache_hits;

/* Structure that defines the cache block */
typedef struct {
   unsigned int tag;
   unsigned int valid;
} CacheBlock;

/* Structure that defines the cache set */
typedef struct {
   CacheBlock block[4];
   uint8_t lru_tree;
} CacheSet_4_way;

typedef struct {
   CacheBlock block[2];
   uint8_t lru_tree;
} CacheSet_2_way;

typedef struct {
   CacheBlock block[1];
} CacheSet_DM;


/* Allocate memory for the Data Cache*/
extern CacheSet_4_way *dcache_4_way;
extern CacheSet_2_way *dcache_2_way;
extern CacheSet_DM *dcache_DM;

void initialize(FILE *fp);
void process_instructions();