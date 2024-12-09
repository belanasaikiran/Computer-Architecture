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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "instruction_map.h"
#include "register_map.h"
#include "sim_stages.h"
#include "sim_core.h"
#include "util.h"

/**
 * Dump register contents.
 * Will format for desired number of columns and output in specified file.
 */
void rdump_file_columns(FILE* file, unsigned columns) {
    static const unsigned int index_col_width = 4;
    static const unsigned int name_col_width = 5;
    static const unsigned int value_col_width = 8;
    static const unsigned int tab_spaces = 4;
    static const unsigned int col_sep = 2;

    assert(columns > 0);

    /* Calculate number of rows and total row length*/
    const unsigned int rows = (int)ceil((double) MAX_LENGTH / columns);
    const unsigned int row_length = columns * (index_col_width + name_col_width + value_col_width + 2 + 2 * tab_spaces) + (columns - 1) * (col_sep * tab_spaces);

    /* Print header */
    fprintf(file, "---------------------\n");
    fprintf(file, "--- Register Dump ---\n");
    fprintf(file, "---------------------\n");
    for (int col = 0; col < columns; col++) {
        fprintf(file, "%-*s%-*s%-*s", index_col_width + tab_spaces, "Index", name_col_width + tab_spaces, "Name", value_col_width + 2, "Value");
        if (col == columns - 1) {
            fprintf(file, "\n");
        } else {
            fprintf(file, "%*s", col_sep * tab_spaces, "");
        }
    }
    for (int col = 0; col < columns; col++) {
        fprintf(file, "%-*s%-*s%-*s", index_col_width + tab_spaces, "-----", name_col_width + tab_spaces, "----", value_col_width + 2, "-----");
        if (col == columns - 1) {
            fprintf(file, "\n");
        } else {
            fprintf(file, "%*s", col_sep * tab_spaces, "");
        }
    }

    /* Print rows */
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            unsigned int i = row + col * rows;

            if (i < MAX_LENGTH) {
                fprintf(file, "x%-*i%*s%-*s%*s 0x%0*X", index_col_width, i, tab_spaces - 1, "", name_col_width, register_map[i], tab_spaces - 1, "", value_col_width, registers[i]);
            } else {
                fprintf(file, "\n");
                break;
            }

            if (col == columns - 1) {
                fprintf(file, "\n");
            } else {
                fprintf(file, "%*s", col_sep * tab_spaces, "");
            }
        }
    }
    fprintf(file, "%-*s%*s%-*s%*s0x%08X\n", index_col_width, "N/A", tab_spaces, "", name_col_width, "pc", tab_spaces, "", pc);
}

void rdump_pt() {
    rdump_file_columns(fptr_pt, 4);
}

void rdump() {
    rdump_file_columns(stdout, 4);
}

/**
 * Dump memory contents.
 */
void mdump() {
    FILE* fptr;
    fptr = fopen("mdump.txt", "w");
    int i = 0;
    for (i = 0; i < 16384; i++) {
        fprintf(fptr, "Memory[%d] = 0x%08X\n", i, memory[i]);
    }
    fclose(fptr);
}

void mdump_modified() {
    FILE* fptr = stdout;
    fprintf(fptr, "\n----------------------------\n");
    fprintf(fptr, "--- Updated Memory Dump ---\n");
    fprintf(fptr, "----------------------------\n");
    int i = 0;
    for (i = 256; i < 16384; i++) { // only dump modified data memory
        if (memory[i] == 0 || memory[i] == -1) {}
        else {
            fprintf(fptr, "Memory[%d] = 0x%08X\n", i, memory[i]);
        }
    }
}

/**
 * Print Branch Information
 */
void bdump(){
    FILE *fptr;
    fptr = fopen("bdump.txt", "w");
    fprintf(fptr, "%s\n%s   %s   %s   %s   %s\n%s\n",
    "**** Branch Target Buffer Info ****",
    "Index","Inst Type","Address","Target Address","Valid",
    "-----   ---------   -------   --------------   -----");
    for (int i = 0; i < BTB_SIZE; i++){
        int instruction = memory[btb[i].inst_addr/4];
        char inst_type[8];
        if (btb[i].valid){
            unsigned f3 = (instruction >> 12) & 0x7;
            switch(f3){
                case 0b000:
                    strcpy(inst_type, "beq");
                    break;
                case 0b001:
                    strcpy(inst_type, "bne");
                    break;
                case 0b100:
                    strcpy(inst_type, "blt");
                    break;
                case 0b101:
                    strcpy(inst_type, "bge");
                    break;
            }
        }else{
            strcpy(inst_type,  "");
        }
        fprintf(fptr, "%5d   %9s   %7d   %14d   %5d\n",
        i, inst_type, btb[i].inst_addr, btb[i].branch_target, btb[i].valid);
    }
    fprintf(fptr, "\n\n%s\n%s   %s\n%s\n",
    "**** Branch History Info ****",
    "Bit Pattern","Predictor State",
    "-----------   ---------------");
    char pattern_buff[5], state_buff[3];
    for (int i = 0; i < BHT_SIZE; i++){
        getBin(i, pattern_buff, 4);
        getBin(bht[i], state_buff, 2);
        fprintf(fptr, "%11s   %15s\n",
        pattern_buff, state_buff);
    }
    fclose(fptr);   
}

/**
 * Print cache information
 */
void cdump() {
    
    if (dcache_enabled)
    {
        FILE *fptr;
        fptr = fopen("cdump.txt","w");
        int num_blocks;
        int num_sets;
        if (dcache_enabled == 1)        num_blocks = 1;
        else if (dcache_enabled == 2)   num_blocks = 2;
        else if (dcache_enabled == 3)   num_blocks = 4;
        num_sets = ((CACHE_SIZE/CACHE_LINE_SIZE)/num_blocks);
        /* Create the set # headers*/
        fprintf(fptr, "%23s", "");
        for (int i = 0; i < num_blocks; i++){
            fprintf(fptr, "%9s %d    ", "Block", i);
        }
        fprintf(fptr, "\n");
        
        /* Create the cache line headers*/
        fprintf(fptr, "%s  %s   ", "Set No.","LRU b2|b1|b0");
        for (int i = 0; i < num_blocks; i++){
            fprintf(fptr, "%s  %s     ","Valid","Tag");
        }
        fprintf(fptr, "\n");

        /* Formatting */
        fprintf(fptr, "%s", "-------  ------------   ");
        for (int i = 0; i < num_blocks; i++){
            fprintf(fptr, "%s","-----  ---     ");
        }
        fprintf(fptr, "\n");

        /* Print the relevent data*/
        char lru_s[4];
        for (int i = 0; i < num_sets; i++){

            if (dcache_enabled == 3) getBin(dcache_4_way[i].lru_tree, lru_s, 3);
            else if (dcache_enabled == 2) getBin(dcache_2_way[i].lru_tree, lru_s, 3);

            if (dcache_enabled == 3 || dcache_enabled == 2)fprintf(fptr, "%7d       %c  %c  %c   ", i, lru_s[0], lru_s[1], lru_s[2]);
            else if (dcache_enabled == 1) fprintf(fptr, "%7d       %d  %d  %d   ", i, 0,0,0);
            for (int j = 0; j < num_blocks; j++){
                if (dcache_enabled == 3) fprintf(fptr, "%5d  %3d     ", dcache_4_way[i].block[j].valid, dcache_4_way[i].block[j].tag);
                else if (dcache_enabled == 2) fprintf(fptr, "%5d  %3d     ", dcache_2_way[i].block[j].valid, dcache_2_way[i].block[j].tag);
                else if (dcache_enabled == 1) fprintf(fptr, "%5d  %3d     ", dcache_DM[i].block[j].valid, dcache_DM[i].block[j].tag);
            }
            fprintf(fptr, "\n");
        }
        fclose(fptr);

    }

    
}

/**
 * Print intructions
*/
void inst_dump(const char stage[], const unsigned int inst) {
    int op = inst & 0x7F;
    int funct3 = (inst >> 12) & 0x7;
    int funct7 = (inst >> 25) & 0x7F;
    int rd = (inst >> 7) & 0x1F;
    int rs1 = (inst >> 15) & 0x1F;
    int rs2 = (inst >> 20) & 0x1F;
    char rds[5], rs1s[5], rs2s[5];
    int show_name = pipe_trace_mode & 0b01;
    int show_dec = ((pipe_trace_mode & 0b10) >> 1);

    if (show_name){
        sprintf(rds, "%s", register_map[rd]);
        sprintf(rs1s, "%s", register_map[rs1]);
        sprintf(rs2s, "%s", register_map[rs2]);
    }else{
        sprintf(rds, "x%d", rd);
        sprintf(rs1s, "x%d", rs1);
        sprintf(rs2s, "x%d", rs2);
    }

    char istring[6];
    getInstStr(op, funct3, funct7, istring);
    int iimm = (signed)(inst & 0xFFF00000) >> 20;
    int simm = ((signed)(inst & 0xFE000000) >> 20) | 
               (        (inst & 0x00000F80) >> 7);  
    int bimm = ((signed)(inst & 0x80000000) >> 19) |
               (        (inst & 0x7E000000) >> 20) |
               (        (inst & 0x00000F00) >>  7) |
               (        (inst & 0x00000080) <<  4); 
    int uimm = (inst & 0xFFFFF000) >> 12;
    int jimm = ((signed)(inst & 0x80000000) >> 11) | 
               (        (inst & 0x7FE00000) >> 20) | 
               (        (inst & 0x00100000) >> 9)  |
               (        (inst & 0x000FF000));  

    char iimms[32], simms[32], bimms[32], uimms[32], jimms[32];
    if (show_dec){
        sprintf(iimms, "%d", iimm);
        sprintf(simms, "%d", simm);
        sprintf(bimms, "%d", bimm);
        sprintf(uimms, "%d", uimm);
        sprintf(jimms, "%d", jimm);
    }else{
        sprintf(iimms, "0x%X", (unsigned)iimm);
        sprintf(simms, "0x%X", (unsigned)simm);
        sprintf(bimms, "0x%X", (unsigned)bimm);
        sprintf(uimms, "0x%X", (unsigned)uimm);
        sprintf(jimms, "0x%X", (unsigned)jimm);
    }

    fprintf(fptr_pt, "%-12s ", stage);

    switch (op) {
        case 0b0110011: 
            fprintf(fptr_pt, "%-4s %s, %s, %s\n", istring, rds, rs1s, rs2s);
            break;

        case 0b1100111: 
            iimm &= ~0b1;
        case 0b0010011:
            fprintf(fptr_pt, "%-4s %s, %s, %s\n", istring, rds, rs1s, iimms);
            break;

        case 0b0100011: 
            fprintf(fptr_pt, "%-4s %s, %s(%s)\n", istring, rs2s, simms, rs1s);
            break;

        case 0b0000011: 
            fprintf(fptr_pt, "%-4s %s, %s(%s)\n", istring, rds, iimms, rs1s);
            break;
        
        case 0b1100011: 
            fprintf(fptr_pt, "%-4s %s, %s, %s\n", istring, rs1s, rs2s, bimms);
            break;

        case 0b0110111: 
            fprintf(fptr_pt, "%-4s %s, %s\n", istring, rds, uimms);
            break;

        case 0b1101111: 
            fprintf(fptr_pt, "%-4s %s, %s\n", istring, rds, jimms);
            break;

        default:
            fprintf(fptr_pt, "INVALID INSTRUCTION\n");
            break;
    }
}


/**
 * Print intructions
*/
void inst_dump_parallel(const char stage[], const unsigned int inst) {
    int op = inst & 0x7F;
    int funct3 = (inst >> 12) & 0x7;
    int funct7 = (inst >> 25) & 0x7F;
    int rd = (inst >> 7) & 0x1F;
    int rs1 = (inst >> 15) & 0x1F;
    int rs2 = (inst >> 20) & 0x1F;
    char rds[5], rs1s[5], rs2s[5];
    int show_name = pipe_trace_mode & 0b01;
    int show_dec = ((pipe_trace_mode & 0b10) >> 1);

    if (show_name){
        sprintf(rds, "%s", register_map[rd]);
        sprintf(rs1s, "%s", register_map[rs1]);
        sprintf(rs2s, "%s", register_map[rs2]);
    }else{
        sprintf(rds, "x%d", rd);
        sprintf(rs1s, "x%d", rs1);
        sprintf(rs2s, "x%d", rs2);
    }

    char istring[6];
    getInstStr(op, funct3, funct7, istring);
    int iimm = (signed)(inst & 0xFFF00000) >> 20;
    int simm = ((signed)(inst & 0xFE000000) >> 20) | 
               (        (inst & 0x00000F80) >> 7);  
    int bimm = ((signed)(inst & 0x80000000) >> 19) |
               (        (inst & 0x7E000000) >> 20) |
               (        (inst & 0x00000F00) >>  7) |
               (        (inst & 0x00000080) <<  4); 
    int uimm = (inst & 0xFFFFF000) >> 12;
    int jimm = ((signed)(inst & 0x80000000) >> 11) | 
               (        (inst & 0x7FE00000) >> 20) | 
               (        (inst & 0x00100000) >> 9)  |
               (        (inst & 0x000FF000));  

    char iimms[32], simms[32], bimms[32], uimms[32], jimms[32];
    if (show_dec){
        sprintf(iimms, "%d", iimm);
        sprintf(simms, "%d", simm);
        sprintf(bimms, "%d", bimm);
        sprintf(uimms, "%d", uimm);
        sprintf(jimms, "%d", jimm);
    }else{
        sprintf(iimms, "0x%X", (unsigned)iimm);
        sprintf(simms, "0x%X", (unsigned)simm);
        sprintf(bimms, "0x%X", (unsigned)bimm);
        sprintf(uimms, "0x%X", (unsigned)uimm);
        sprintf(jimms, "0x%X", (unsigned)jimm);
    }

    fprintf(fptr_pt, "%-12s ", stage);

    switch (op) {
        case 0b0110011: 
            fprintf(fptr_pt, "%-4s %s, %s, %s           ", istring, rds, rs1s, rs2s);
            break;

        case 0b1100111: 
            iimm &= ~0b1;
        case 0b0010011:
            fprintf(fptr_pt, "%-4s %s, %s, %s           ", istring, rds, rs1s, iimms);
            break;

        case 0b0100011: 
            fprintf(fptr_pt, "%-4s %s, %s(%s)           ", istring, rs2s, simms, rs1s);
            break;

        case 0b0000011: 
            fprintf(fptr_pt, "%-4s %s, %s(%s)           ", istring, rds, iimms, rs1s);
            break;
        
        case 0b1100011: 
            fprintf(fptr_pt, "%-4s %s, %s, %s           ", istring, rs1s, rs2s, bimms);
            break;

        case 0b0110111: 
            fprintf(fptr_pt, "%-4s %s, %s               ", istring, rds, uimms);
            break;

        case 0b1101111: 
            fprintf(fptr_pt, "%-4s %s, %s               ", istring, rds, jimms);
            break;

        default:
            fprintf(fptr_pt, "INVALID INSTRUCTION               ");
            break;
    }
}


void getInstStr(int op, int f3, int f7, char *buffer){
    switch(op){
        case 0b0110011:
            switch(f3){
                case 0b000:
                    if (f7) strcpy(buffer, "sub"); else strcpy(buffer, "add");
                    break;
                case 0b001:
                    strcpy(buffer, "sll");
                    break;
                case 0b101:
                    strcpy(buffer, "srl");
                    break;
                case 0b111:
                    strcpy(buffer, "and");
                    break;
                case 0b110:
                    strcpy(buffer, "or");
                    break;
                case 0b100:
                    strcpy(buffer, "xor");
                    break;
                case 0b010:
                    strcpy(buffer, "slt");
                    break;
            }
            break;
        case 0b0010011:
            switch(f3){
                case 0b000:
                    strcpy(buffer, "addi");
                    break;
                case 0b010:
                    strcpy(buffer, "slti");
                    break;
                case 0b111:
                    strcpy(buffer, "andi");
                    break;
                case 0b110:
                    strcpy(buffer, "ori");
                    break;
                case 0b100:
                    strcpy(buffer, "xori");
                    break;
                case 0b001:
                    strcpy(buffer, "slli");
                    break;
                case 0b101:
                    strcpy(buffer, "srli");
                    break;
            }
            break;

        case 0b1100111:
            strcpy(buffer, "jalr");
            break; 

        case 0b0000011:
            strcpy(buffer, "lw");
            break;

        case 0b1100011:
            switch(f3){
                case 0b000:
                    strcpy(buffer, "beq");
                    break;
                case 0b001:
                    strcpy(buffer, "bne");
                    break;
                case 0b100:
                    strcpy(buffer, "blt");
                    break;
                case 0b101:
                    strcpy(buffer, "bge");
                    break;
            }
            break;
        
        case 0b100011:
            strcpy(buffer, "sw");
            break;
    
        case 0b1101111:
            strcpy(buffer, "jal");
            break;

        case 0b0110111:
            strcpy(buffer, "lui");
            break;
        
        default:
            break;
    }
}


/**
 * Convert a decimal string to binary
 */
void getBin(int num, char *str, int size) {
	int n = num;
	char *lsb = str;
	for (int i = 0; i < size; i++){
		if (n & 1) lsb[size-i-1] = '1'; else lsb[size-i-1] = '0';
		n >>= 1;
	}
	str[size] = '\0';
}

/**
 * Convert a binary string to an integer
 */
int getDec(char *bin) {
    int b, k, m, n;
    int len, sum = 0;

    len = strlen(bin) - 1;

    /* Iterate over the string */
    for (k = 0; k <= len; k++) {
        // Convert char to numeric value
        n = (bin[k] - '0');

        // Check the character is binary
        if ((n > 1) || (n < 0)) {
            return 0;
        }

        for (b = 1, m = len; m > k; m--)
            b *= 2;

        // sum it up
        sum = sum + n * b;
    }
    return sum;
}