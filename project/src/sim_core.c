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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_map.h"
#include "register_map.h"
#include "sim_core.h"
#include "sim_stages.h"
#include "util.h"

/**
 * Initial CPU state
 */
int registers[MAX_LENGTH] = {0};    // Registers
unsigned int pc = 0;                // Program Counter (PC) register
unsigned int pc_n = 0;              // Next Program Counter
int *memory = NULL;                 // Data & instruction memory

/**
 * Utility
 */
FILE *fptr_pt;

/* Set the nop variable */
const struct State nop = {.inst = 0x0000000013};

/* Different settings that can be enabled */
int forwarding_enabled  = 0;
int ooo_enabled  = 0;
int branch_prediction_enabled = 0;
int dcache_enabled = 0;


/* Initialize pipeline state */
struct State fetch_out,  fetch_out_n;
struct State decode_out, decode_out_n;
struct State ex_out,     ex_out_n;
struct State ex_ld_st_out, ex_ld_st_out_n;
struct State ex_ld_st_2_out, ex_ld_st_2_out_n;
struct State wb_out, wb_out_n;
struct State wb_ld_st_out, wb_ld_st_out_n;
struct State wb_ld_st_2_out, wb_ld_st_2_out_n;


/* Pipeline-related initialization */
int pipe_stall          = 0;
int j_taken             = 0; 
int we_exe              = 0,   ws_exe         = 0, dout_exe         = 0;
int we_mem              = 0,   ws_mem         = 0, dout_mem         = 0;
int we_mem2             = 0,   ws_mem2        = 0, dout_mem2        = 0;
int we_wb               = 0,   ws_wb          = 0, dout_wb          = 0;
int we_ld_st_wb         = 0,   ws_ld_st_wb    = 0, dout_ld_st_wb    = 0;
int we_ld_st_2_wb       = 0,   ws_ld_st_2_wb  = 0, dout_ld_st_2_wb  = 0;

/* Multi-cycle operation-related initialization */
const int dmem_access_cycles = 6;
const int dcache_access_cycles = 2;
int dmem_busy = 0;
int dmem_cycles = 0;
int dmem_busy2 = 0;
int dmem_cycles2 = 0;

/* Branch Related */
BranchTargetBuffer *btb;
enum PREDICTION *bht;
uint8_t bhsr;
int br_mispredicted = 0;
int total_branches = 0;
int correctly_predicted_branches = 0;

/* Data cache-related */
CacheSet_4_way *dcache_4_way;
CacheSet_2_way *dcache_2_way;
CacheSet_DM *dcache_DM;
int dmem_accesses = 0;
int dcache_hits = 0;

/* Count all non-nop instructions */
int instruction_counter = 0;  
 

/* Total Cycles */
int cycle = 0;     

/* BHSR/BHT info */
char BHSR_string[5];
char BHT_string[3];

/**
 * Simulator entry point
 */
int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "[ERROR] incorrect number of arguments.\n");
        printf("usage: simulator PROGRAM_FILE FORWARDING_ENABLED OOO_ENABLED DYNAMIC_BP_ENABLED DATA_CACHE_ENABLED\n");
        exit(1);
    } else {
        /* Open input program file */
        FILE *fp;
        fp = fopen(argv[1], "r");

        /* Enable/disable forwarding */
        sscanf(argv[2],"%d", &forwarding_enabled);
        if (forwarding_enabled > 1) {
            fprintf(stderr, "[ERROR] FORWARDING_ENABLED must be either 0 (disabled) or 1 (enabled).\n");
            exit(1);
        }
        printf("Pipeline Forwarding: %s\n", forwarding_enabled ? "Enabled" : "Disabled");

        /* Enable/disable OOO */
        sscanf(argv[3],"%d", &ooo_enabled);
        if (ooo_enabled > 1) {
            fprintf(stderr, "[ERROR] OOO_ENABLED must be either 0 (disabled) or 1 (enabled).\n");
            exit(1);
        }
        printf("OOO Execution: %s\n", ooo_enabled ? "Enabled" : "Disabled");

        /* Enable/disable Branch Predictor */
        sscanf(argv[4], "%d", &branch_prediction_enabled);
        if (branch_prediction_enabled > 2) {
            fprintf(stderr, "[ERROR] DYNAMIC_BP_ENABLED must be either 0 (Not Taken Predictor) or 1 (Single Level Predictor) or 2 (Two Level Predictor).\n");            
            exit(1);
        }
        printf("Dynamic Branch Prediction: %s\n", branch_prediction_enabled ? "Enabled" : "Disabled");

        /* Enable/disable Data cache */
        sscanf(argv[5], "%d", &dcache_enabled);
        if (dcache_enabled > 3) {
            fprintf(stderr, "[ERROR] DATA_CACHE_ENABLED must be either 0 (disabled) or 1 (Direct Mapped) or 2 (2-way Set Associative) or 3 (4-way Set Associative).\n");            
            exit(1);
        }
        printf("Data Cache: %s\n", dcache_enabled ? "Enabled" : "Disabled");

        
        /* Open pipe trace */
        if (pipe_trace) {
            fptr_pt = fopen("pipe_trace.txt", "w");
        }

        /* Initialize registers and instruction/data memory */
        initialize(fp);

        puts("\n");
        printf("Simulating...\n");

        /* Process instructions one at a time */
        process_instructions();

        puts("");

        /* Output state after termination */
        rdump();    // Register dump
        mdump_modified();
        mdump();    // Memory dump
        bdump();    // Branch prediction dump
        cdump();    // Data cache dump
        
        /* Cleanup */
        free(memory);
        fclose(fp);
        if (pipe_trace) {
            fclose(fptr_pt);
        }

        return 0;
    }
}

void process_instructions() {
    int terminate = 0;
    unsigned int committed_inst;
    unsigned int committed_inst_ld_st;
    while (terminate != 1) {
        
        /* Update pipeline state */
        
                                  writeback();
        ex_out_n                = execute();
        ex_ld_st_out_n          = execute_ld_st();
        ex_ld_st_2_out_n        = execute_2nd_ld_st();
        decode_out_n            = decode();
        fetch_out_n             = fetch();
        

        //increment cycle counter
        cycle++;

        /* Increment the total instruction counter */
        if (wb_out_n.inst != nop.inst){
            instruction_counter++;
        }

        if (ooo_enabled)
        {
            /* Increment the total instruction counter */
            if (wb_ld_st_out_n.inst != nop.inst){
                instruction_counter++;
            }
            if (wb_ld_st_2_out_n.inst != nop.inst){
                instruction_counter++;
            }
        }

        if (pipe_trace == 1) {
            fprintf(fptr_pt, "Cycle %d, PC %d, Next PC %d\n", cycle, pc, pc_n);
            inst_dump("[Fetch]",     fetch_out_n.inst);
            inst_dump("[Decode]",    decode_out_n.inst);
            inst_dump_parallel("[Execute ALU]",   ex_out_n.inst);
            
            inst_dump_parallel("   [Execute LD ST]",   ex_ld_st_out_n.inst);
            inst_dump("   [Execute LD ST2]",   ex_ld_st_2_out_n.inst);

            if (ooo_enabled) {
                inst_dump_parallel("[Writeback ALU]",    wb_out_n.inst);
                inst_dump_parallel("[Writeback LD ST]", wb_ld_st_out_n.inst);
                inst_dump("     [Writeback LD ST2]", wb_ld_st_2_out_n.inst);
            }
            else {
                inst_dump_parallel("[Writeback]",    wb_out_n.inst);

            }    
            

            fprintf(fptr_pt, "\n");
            rdump_pt();
            fprintf(fptr_pt, "\n");
            fprintf(fptr_pt, "=================================================================================================================================\n");
            fprintf(fptr_pt, "\n");
        }

        if (debug) {
            fprintf(stderr, "[DEBUG] Cycle: %d, Instruction Memory Address: %d, Instruction: 0x%08x\n", cycle, pc / 4, fetch_out_n.inst);
        }

        if (debug) {
            fprintf(stderr, "[DEBUG] Cycle: %d, Committed Instruction: 0x%08x\n", cycle, committed_inst);
        }

        if (registers[0] != 0) {
            terminate = 1;  // set terminate flag when $zero is updated
        }

        /* Update state for next cycle */
        pc              = pc_n;
        decode_out      = decode_out_n;
        fetch_out       = fetch_out_n;
        ex_out          = ex_out_n;
        ex_ld_st_out    = ex_ld_st_out_n;
        ex_ld_st_2_out  = ex_ld_st_2_out_n;
        wb_out          = wb_out_n;
        wb_ld_st_out    = wb_ld_st_out_n;
        
        
        if (cycle == 25000) {
            fprintf(stderr, "\n[WARNING] Simulation has simulated 25,000 cycles without terminating. Something might be wrong. Terminating.\n");
            exit(1);
        }
    }

    float br_correct_predictions = 0, dcache_hitrate = 0;

    if (total_branches){
        br_correct_predictions = ((float)correctly_predicted_branches/(float)total_branches)*100;
    }

    if (dmem_accesses){
        dcache_hitrate = ((float)dcache_hits/(float)dmem_accesses)*100;
    }

    printf("\nFinished simulation!\n");
    printf("\nTOTAL INSTRUCTIONS COMMITTED: %d\n", instruction_counter);
    printf("TOTAL CYCLES SIMULATED: %d\n", cycle);
    printf("AVERAGE CPI: %0.3f\n\n", (double)cycle/(double)instruction_counter);

    printf("TOTAL CONDITIONAL BRANCHES: %d\n", total_branches);
    printf("BRANCHES CORRECTLY PREDICTED: %d\n", correctly_predicted_branches);
    printf("BRANCH PREDICTION ACCURACY: %.2f%%\n\n", br_correct_predictions);

    printf("TOTAL MEMORY ACCESSES: %d\n", dmem_accesses);
    printf("CACHE HITS: %d\n", dcache_hits);
    printf("CACHE HIT RATE: %.2f%%\n", dcache_hitrate);

   
}

void initialize(FILE *fp) {
    printf("======================================\n");
    printf("=== BEGIN SIMULATOR INITIALIZATION ===\n");
    printf("======================================\n");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] opening input file. Aborting.\n");
        exit(1);
    }

    /* Initialize the starting states */
    fetch_out       = nop;
    decode_out      = nop;
    ex_out          = nop;
    ex_ld_st_out    = nop;
    ex_ld_st_2_out  = nop;
    wb_out          = nop;
    wb_ld_st_out    = nop;
    wb_ld_st_2_out    = nop;

    fetch_out_n       = nop;
    decode_out_n      = nop;
    ex_out_n          = nop;
    ex_ld_st_out_n    = nop;
    ex_ld_st_2_out_n  = nop;
    wb_out_n          = nop;
    wb_ld_st_out_n    = nop;
    wb_ld_st_2_out_n   = nop;
    
    /* Zero initialize registers */
    memset(registers, 0, sizeof(registers));
    printf("Initialized Registers\n");
    puts("");

    /* Allocate instruction and data memory */
    memory = (int*) malloc(16384 * sizeof(int));
    if (memory == NULL) {
        fprintf(stderr, "[ERROR] not enough memory for data memory. Aborting.\n");
        exit(1);
    }

    /* Initialize instruction memory to "nop" -1 */
    for (int i = 0; i < 256; i++) {
        memory[i] = nop.inst;
    }

    /* Initialize data memory to "nop" -1 */
    for (int i = 256; i < 16384; i++) {
        memory[i] = -1;
    }

    printf("Initialized Memory\n");
    puts("");

    /* Allocate data cache */
    dcache_4_way = (CacheSet_4_way*) calloc(((CACHE_SIZE/CACHE_LINE_SIZE)/4), sizeof(CacheSet_4_way));
    dcache_2_way = (CacheSet_2_way*) calloc(((CACHE_SIZE/CACHE_LINE_SIZE)/2), sizeof(CacheSet_2_way));
    dcache_DM = (CacheSet_DM*) calloc(((CACHE_SIZE/CACHE_LINE_SIZE)/1), sizeof(CacheSet_DM));
    
    if (dcache_4_way == NULL) {
        fprintf(stderr, "[ERROR] not enough memory for 4-way cache. Aborting.\n");
        exit(1);
    }

    if (dcache_2_way == NULL) {
        fprintf(stderr, "[ERROR] not enough memory for 2-way cache. Aborting.\n");
        exit(1);
    }

    if (dcache_DM == NULL) {
        fprintf(stderr, "[ERROR] not enough memory for Direct Mapped cache. Aborting.\n");
        exit(1);
    }

    printf("Initialized Data Cache\n");
    puts("");

    /* Allocate and zero-initialize target buffer */
    btb = (BranchTargetBuffer *) calloc(BTB_SIZE, sizeof(BranchTargetBuffer));
    bht = (enum PREDICTION *) calloc(BHT_SIZE, sizeof(enum PREDICTION));
    

    /* Initialize the BHT */
    for (int i = 0; i < BHT_SIZE; i++){
        bht[i] = N;
    }

    bhsr = 0;
    if (btb == NULL || bht == NULL){
        fprintf(stderr, "[ERROR] not enough memory for dynamic branch prediction. Aborting.\n");
        exit(1);
    }
    printf("Initialized BTB, BHT, and BHSR.\n");
    puts("");

    printf("----------------------\n");
    printf("--- Section: .text ---\n");
    printf("----------------------\n");

    /* Initialize parsing variables */
    char line[MAX_LENGTH + 2];
    char *p;
    int i = 0, line_num = 0;

    /* Copy .text section to memory, break at nop */
    while (fgets(line, MAX_LENGTH + 2, fp) != NULL) {
        line_num++;

        /* Remove '\n' from 'line' */
        p = strchr(line, '\n');
        if (p != NULL) {
            *p = '\0';
        }

        memory[i] = getDec(line);

        /* If 'nop' found, move to 0x8000 / 2048 in memory and break */
        if (strcmp(line, "11111111111111111111111111111111") == 0) {
            memory[i] = nop.inst;
            i = 256; //Data Memory Starts at 256
            break;
        } else {
            printf("memory[%d] = 0x%08x\n", i, memory[i]);
            i++;
        }
    }
    int j;              
    for (j = i; j < 16384; j++) {
        memory[j] = 0;
    }

    puts("");

    printf("----------------------\n");
    printf("--- Section: .data ---\n");
    printf("----------------------\n");

    /* Seek fp to first instruction in .data */
    char data[MAX_LENGTH + 2];
    int bytes = 33 * line_num;
    fseek(fp, bytes, SEEK_SET);

    /* Copy .data section to memory */
    while (fgets(line, MAX_LENGTH + 2, fp) != NULL) {
        /* Remove '\n' from 'line' */
        p = strchr(line, '\n');
        if (p != NULL) {
            *p = '\0';
        }

        memory[i] = getDec(line);
        printf("memory[%d] = 0x%08x\n", i, memory[i]);
        i++;
    }

    printf("====================================\n");
    printf("=== END SIMULATOR INITIALIZATION ===\n");
    printf("====================================");
}
