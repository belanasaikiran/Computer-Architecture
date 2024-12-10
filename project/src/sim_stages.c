/**
 * University of Connecticut
 * CSE 4302: Computer Architecture
 * Fall 2024
 * Hanan Khan (Reference Implementation)
 * 
 * Project: MultiCycle Stall + OOO + Seperate Execution Units + BP + Cache
 * 
 * riscy-uconn: sim_stages.c
 * 
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_map.h"
#include "sim_core.h"
#include "sim_stages.h"
#include "decode_fields.h"

/**
 * Debug flags
 */
int debug = 0;           /* Set to 1 for additional debugging information. */
int pipe_trace = 1;      /* Set to 1 for pipe trace. */
int pipe_trace_mode = 3; /* See PA1 handout, section 5 for usage */

int mylog(int x){
    int result = -1;
    int buf = x;
    while (buf){
        buf = buf >> 1;
        result++;
    }
    return result;
}


/**
 * Fetch stage implementation.
 */
struct State fetch() {

    fetch_out_n.inst = memory[pc / 4];
    fetch_out_n.inst_addr = pc;

    
    /* Insert a NOP if jump is taken / branch is mispredicted */
    if (j_taken || br_mispredicted) {
        return nop;
    }
    else if (pipe_stall){  //this has been modified now we dont stall on a dmem_busy
        return fetch_out;
    } else {

        /* if branch_prediction_enabled is set to 1 or 2, then BTB_lookup() is used to check if the current instruction has a valid entry in the BTB. If so, then predict_direction()
            is used to check the branch direction prediction. For a taken branch, the function BTB_target()
            is used to get the target address, and the br_predicted flag set to ‘1’ to indicate a taken branch
            predicted instruction. Otherwise, this field should be ‘0’.
            In case of the dynamic predictors being enabled, the BTB hit and the BHT direction of ‘taken’ uses
            the BTB target address to update the pc_n using advance_pc() function. Otherwise, the pc_n is
            incremented using the not-taken path.*/
        if(branch_prediction_enabled == 1 || branch_prediction_enabled == 2){
            if (BTB_lookup(pc)) {
                int prediction = predict_direction(pc);
                if (prediction) {
                    pc_n = BTB_target(pc);
                    fetch_out_n.br_predicted = 1;
                } else {
                    pc_n = pc + 4;
                    fetch_out_n.br_predicted = 0;
                }
            } else {
                pc_n = pc + 4;
                fetch_out_n.br_predicted = 0;
                return fetch_out_n;
            }
        } else {
            advance_pc(fetch_out_n.inst_addr + 4);
        }

    }

    return fetch_out_n;
       
}

/**
 * Decode stage implementation
 */
struct State decode() {
    

    // read the fetch_out state and start processing decode functionality 
    decode_out_n = fetch_out;

     /* Reset relevant pipeline variables */
    pipe_stall = 0;
    j_taken = 0;

    
    /* If branch taken (resolved in EX stage) then inject NOP */
    if (br_mispredicted){
        return nop;
    }

    // struct State *ptr = &fetch_out;
    decode_fields(&decode_out_n);

    /* Used to check if rs1 pr rs2 is read */
    int rs1_read = 0;
    int rs2_read = 0;

    /**
     * Determine which registers the current instruction will read
     */
    switch(decode_out_n.opcode){
        /* All R-types, B-types, and S-types use rs1 / rs2 as operands */
        case RTYPE:
        case BTYPE:
        case STYPE:
            rs1_read = 1; 
            rs2_read = 1;
            break;

        /* All I-types only read from rs1 */
        case ITYPE_ARITH:
        case ITYPE_LOAD:
        case JALR:
            rs1_read = 1;
        
        /* Other instructions don't do reads on rs1 / rs2*/
        default:
            break;
    }

    /**
     * Register source values. They initialize to the register file, 
     * but will be set to the most up-to-date values.
    */
    int rs1_source = registers[decode_out_n.rs1];
    int rs2_source = registers[decode_out_n.rs2];
    int raw_hazard = 0;
    
    /**
     * Pipeline interlock checks
     */
    /* rs1 register (make sure we are reading from rs1 and that the register is not x0!)*/
    if (rs1_read == 1 && decode_out_n.rs1 != 0)
    {
        /* Execute stage of Non Load Store */
        if (we_exe && (decode_out_n.rs1 == ws_exe)) {
            rs1_source = dout_exe;
            raw_hazard = 1;
        }

        /* Execute stage of LOAD STORE doesn't forward data*/
        
        /* Memory stage */
        else if (we_mem && (decode_out_n.rs1 == ws_mem)) {
            rs1_source = dout_mem;                  /*this will only be valid if dmem_busy is set to 0*/
            raw_hazard = 1;
             if (dmem_busy)
            {
                pipe_stall = 1;                // stall if there is a multicycle Load/store so forwarding does not occur
                return nop;
            }
        }
        // OK: 2nd Ld/St unit
        else if (we_mem2 && (decode_out_n.rs1 == ws_mem2)) {
            rs1_source = dout_mem2;                  /*this will only be valid if dmem_busy is set to 0*/
            raw_hazard = 1;
             if (dmem_busy2)
            {
                pipe_stall = 1;                // stall if there is a multicycle Load/store so forwarding does not occur
                return nop;
            }
        }

        /* Writeback stage of Non load store*/
        /*if ooo not enabled then wb stage of all instructions*/
        else if (we_wb && (decode_out_n.rs1 == ws_wb)) {
            rs1_source = dout_wb;
            raw_hazard = 1;
        }

        if (ooo_enabled)
        {
            /* Writeback stage of load store*/
            if (we_ld_st_wb && (decode_out_n.rs1 == ws_ld_st_wb)) {
                rs1_source = dout_ld_st_wb;
                raw_hazard = 1;
            }

            // OK: WB of 2nd Ld/St unit
            if (we_ld_st_2_wb && (decode_out_n.rs1 == ws_ld_st_2_wb)) {
                rs1_source = dout_ld_st_2_wb;
                raw_hazard = 1;
            }

        }

        

    }

    /* rs2 register (make sure we are reading from rs2 and that the register is not x0!)*/
    if (rs2_read == 1 && decode_out_n.rs2 != 0) {
        /* Execute stage of ALU*/
        if (we_exe && (decode_out_n.rs2 == ws_exe)) {
            rs2_source = dout_exe;
            raw_hazard = 1;
        }

        /* Execute stage of LOAD STORE doesnt forward */
       

        /* Memory stage */
        else if (we_mem && (decode_out_n.rs2 == ws_mem)) {
            rs2_source = dout_mem;                  /*this will only be valid if dmem_busy is set to 0*/
            raw_hazard = 1;
            if (dmem_busy)
            {
                pipe_stall = 1;                      // stall if there is a multicycle Load/store so forwarding does not occur
                return nop;
            }
        }

        // OK: 2nd Ld/St unit
        else if (we_mem2 && (decode_out_n.rs2 == ws_mem2)) {
            rs2_source = dout_mem2;                  /*this will only be valid if dmem_busy is set to 0*/
            raw_hazard = 1;
             if (dmem_busy2)
            {
                pipe_stall = 1;                // stall if there is a multicycle Load/store so forwarding does not occur
                return nop;
            }
        }

        /* Writeback stage of ALU */
        /*if ooo not enabled then wb stage of all instructions*/
        else if (we_wb && (decode_out_n.rs2 == ws_wb)) {
            rs2_source = dout_wb;
            raw_hazard = 1;
        }

        if (ooo_enabled)
        {
            /* Writeback stage of load store*/
            if (we_ld_st_wb && (decode_out_n.rs2 == ws_ld_st_wb)) {
                rs2_source = dout_ld_st_wb;
                raw_hazard = 1;
            }

            // OK: WB of 2nd Ld/St unit
            if (we_ld_st_2_wb && (decode_out_n.rs2 == ws_ld_st_2_wb)) {
                rs2_source = dout_ld_st_2_wb;
                raw_hazard = 1;
            }

        }

    }

    int ld_st_addr = 0;

    if (ooo_enabled)
    {

        // //checks for WAW between Load Store and other instructions 
        //(if we have a Load followed by a Non-Load instruction writing to same  
        //output register, as the Non-Load instruction take a single cycle it will
        //write early because of OOO so to avoid that we stall Non-Load instructions
        //coming after Load instructions that write to the same output register)
        if (!(decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE))
        {
            //this means we have a WAW dependancy
            if (dmem_busy && ws_mem == decode_out_n.rd && we_mem)
            {
                pipe_stall = 1;
                return nop;
            }

            //HK: also add WAW check for second FU
            //this means we have a WAW dependancy
            if (dmem_busy2 && ws_mem2 == decode_out_n.rd && we_mem2)
            {
                pipe_stall = 1;
                return nop;
            }
        }


        if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE)
        {
            ld_st_addr = decode_out_n.imm + rs1_source; // OK: address check not to be supported in Fall 2024
        }

        if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE)
        {
            if (dmem_busy && dmem_busy2)
            {
                pipe_stall = 1;
                return nop;
            }
        }
        
        //handling Store after Load Dependacy (Potential WAR) or Store after Store (potential WAW hazards)
        //If Load takes more cycles to read from memory addr
        //store comes after and writes to same memory addr early 
        if (decode_out_n.opcode == STYPE) {
            if (dmem_busy) { //checking if first FU is busy
                if (((ex_ld_st_out_n.opcode == ITYPE_LOAD) || (ex_ld_st_out_n.opcode == STYPE)) && dmem_cycles == 1) // && ex_ld_st_out_n.mem_addr == ld_st_addr) //if there is a load in first cycle
                {
                    pipe_stall = 1;
                    return nop;
                }
                //if there is a load in the other cycles 
                else if (((ex_ld_st_out_n.opcode == ITYPE_LOAD) || (ex_ld_st_out_n.opcode == STYPE)) && dmem_cycles > 1) // && ex_ld_st_out.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
            }

            //checking if second FU is busy
            if (dmem_busy2)
            {
                //if there is a load in first cycle
                if (((ex_ld_st_2_out_n.opcode == ITYPE_LOAD) || (ex_ld_st_2_out_n.opcode == STYPE)) && dmem_cycles2 == 1) // && ex_ld_st_2_out_n.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
                //if there is a load in the other cycles 
                else if (((ex_ld_st_2_out_n.opcode == ITYPE_LOAD) || (ex_ld_st_2_out_n.opcode == STYPE)) && dmem_cycles2 > 1) // && ex_ld_st_2_out.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
            }
        }


        //handling Load after Store Dependacy (Potential RAW hazard)
        //If Store takes more cycles to write to memory addr
        //Load comes after and reads from same memory addr early 
        if (decode_out_n.opcode == ITYPE_LOAD) {
            if (dmem_busy) {
                //if there is a load in first cycle
                if (ex_ld_st_out_n.opcode == STYPE && dmem_cycles == 1) // && ex_ld_st_out_n.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
                //if there is a load in the other cycles 
                else if (ex_ld_st_out.opcode == STYPE && dmem_cycles > 1) // && ex_ld_st_out.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
            }

            //checking if second FU is busy
            if (dmem_busy2)
            {
                //if there is a load in first cycle
                if (ex_ld_st_2_out_n.opcode == STYPE && dmem_cycles2 == 1) // && ex_ld_st_2_out_n.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
                //if there is a load in the other cycles 
                else if (ex_ld_st_2_out.opcode == STYPE && dmem_cycles2 > 1) // && ex_ld_st_2_out.mem_addr == ld_st_addr)
                {
                    pipe_stall = 1;
                    return nop;
                }
            }
        }


        if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE) {
            /*if both the FU are free then send the Instruction to the first FU*/
            if (dmem_busy == 0 && dmem_busy2 == 0)
            {
                decode_out_n.ld_st_unit = 1;
            }
            
            /*if the first FU is busy then send the Instruction to the second FU*/
            if (dmem_busy == 1 && dmem_busy2 == 0)
            {
                decode_out_n.ld_st_unit = 2;
            }

            /*if the second FU is busy then send the Instruction to the first FU*/
            if (dmem_busy2 == 1 && dmem_busy == 0)
            {
                decode_out_n.ld_st_unit = 1;
            }
        }
        

        /*there is a termination condition after a LW or SW*/
        if (decode_out_n.opcode == ITYPE_ARITH && decode_out_n.rd == 0 && decode_out_n.rs1 == 0 && decode_out_n.imm == 1) {
            //this means we have a structural hazard
            //HK
            if (dmem_busy || dmem_busy2)
            {
                pipe_stall = 1;
                return nop;
            }
        }        
    }

    
    /*If there is a Load followed by another Load or a store followed by another store stall the younger instruction*/
    /*If other Instructions come after Load store stall due to structural hazard*/
    if (ooo_enabled == 0 && dmem_busy == 1)
    {
        pipe_stall = 1;
        return nop; // If the pipeline is stalled then return a nop
    }

   
    /* Stall if a data dependency is detected and forwarding is disabled. We detect a data
    dependency here if either rs_source or rt_source do not equal their initial value. */
    if (forwarding_enabled == 0 && raw_hazard == 1) {
        pipe_stall = 1;
        return nop; // If the pipeline is stalled then return a nop
    }
        
    /**
     * Instruction-specific decode logic (should now read rs1/rs2_source rather than directly from register)
     */
    switch(decode_out_n.opcode){

        /* All R-types use rs1 / rs2 as alu1 / alu2 */
        case RTYPE:
            decode_out_n.alu_in1 = rs1_source;
            decode_out_n.alu_in2 = rs2_source;
            break;

        /* jalr , I-type arithmatic, and load all use upper 12 bits for imm, rs1 for alu1, and imm for alu2 */
        case JALR:
        case ITYPE_ARITH:
        case ITYPE_LOAD:
            decode_out_n.alu_in1 = rs1_source;
            decode_out_n.alu_in2 = decode_out_n.imm;
            
            /* only jalr needs to link the address and change the next pc */ 
            if (decode_out_n.opcode == JALR){
                j_taken = 1; // Make sure to set j_taken for jalr
                decode_out_n.link_addr = decode_out_n.inst_addr + 4; //JALR needs to save the next inst addr for WB
                // pc_n = rs1_source + decode_out_n.imm; //Branch to rs1 + offset
                advance_pc(rs1_source + decode_out_n.imm);
                
            }
            if (decode_out_n.opcode == ITYPE_LOAD && ooo_enabled == 0 ){
                decode_out_n.ld_st_unit = 1; // OK: for now only one load/store unit is scheduled!
            }
            break;
        
        /* S-type needs to decode the immediate accordingly, use rs1 as alu1, imm as alu2, rs2 as word-to-be-saved */
        case STYPE:
            decode_out_n.alu_in1 = rs1_source;
            decode_out_n.alu_in2 = decode_out_n.imm;
            decode_out_n.mem_buffer = rs2_source; //SW needs to save the read register for MEM write
            if (ooo_enabled == 0) decode_out_n.ld_st_unit = 1; // OK: for now only one load/store unit is scheduled!
            break;

        /* B-type need to decode imm accordingly, use rs1 / rs2 as alu1 / alu2, calculate branch address if branch taken */
        case BTYPE:
            decode_out_n.alu_in1 = rs1_source;
            decode_out_n.alu_in2 = rs2_source;
            decode_out_n.br_addr = decode_out_n.inst_addr + decode_out_n.imm; //Branches need to store jump address
            break;

        /* lui does nothing */
        case LUI:
            break;

        /* jal needs to decode imm accordingly, link the address, and change the next pc */
        case JAL:
            j_taken = 1; // Make sure to set j_taken
            decode_out_n.link_addr = decode_out_n.inst_addr + 4; //JALR needs to save the next inst addr for WB
            advance_pc(decode_out_n.inst_addr + decode_out_n.imm);
            break;
    }

    return decode_out_n;

}

/**
 * Execute stage implementation
 */
struct State execute() {
  // read the decode_out state and start processing execute stage's functionality 
    ex_out_n = decode_out;

    /* Reset relevant pipeline variables */    
    we_exe = 0;
    ws_exe = 0;
    dout_exe = 0;

    br_mispredicted = 0;

    // this logic detects if the decoded instruction is being converted to a NOP
    // it also detects if a valid load or store type instruction is being executed
    if (ex_out_n.inst == nop.inst || ex_out_n.opcode == ITYPE_LOAD || ex_out_n.opcode == STYPE) {
        return nop;
    }

    // for NON LOAD or STORE instruction types, perform the execute functionality
    else {
       int br_taken;

        /* Any instruction that writes back does so to the rd register */
        ws_exe = ex_out_n.rd;

        /* Start decoding instructions */
        switch(ex_out_n.opcode){   
            /* Check if it is R-type or arithmatic I-type */
            case RTYPE:
            case ITYPE_ARITH:
                we_exe = 1; // All R-type and I-type arithmatic write to rd 
                switch(ex_out_n.funct3){
                    /* add, addi, or sub */ 
                    case ADD_SUB:
                        /* sub */
                        if (ex_out_n.opcode == RTYPE && ex_out_n.funct3 == ADD_SUB && ex_out_n.funct7){
                            ex_out_n.alu_out = ex_out_n.alu_in1 - ex_out_n.alu_in2;
                        
                        /* add, addi */
                        }else{ 
                            ex_out_n.alu_out = ex_out_n.alu_in1 + ex_out_n.alu_in2;
                        }
                        break; 

                    /* and, andi */
                    case AND:
                        ex_out_n.alu_out = ex_out_n.alu_in1 & ex_out_n.alu_in2;
                        break;

                    /* or, ori */
                    case OR:
                        ex_out_n.alu_out = ex_out_n.alu_in1 | ex_out_n.alu_in2;
                        break;
                    
                    /* xor, xori */
                    case XOR:
                        ex_out_n.alu_out = ex_out_n.alu_in1 ^ ex_out_n.alu_in2;
                        break;
                    
                    /* sll, slli */
                    case SLL:
                        ex_out_n.alu_out = ex_out_n.alu_in1 << ex_out_n.alu_in2;
                        break;
                    
                    /* srl, srli */
                    case SRL:
                        ex_out_n.alu_out = ex_out_n.alu_in1 >> ex_out_n.alu_in2;
                        break;
                    
                    /*slt, slti */
                    case SLT:
                        ex_out_n.alu_out = ex_out_n.alu_in1 < ex_out_n.alu_in2;
                        break;
                }
                break;


            /* bne, beq, blt, bge (all branches compute equalities + branch in EX phase) */
            case BTYPE:
                switch(ex_out_n.funct3)
                {
                    /* beq */
                    case BEQ:
                        br_taken = ex_out_n.alu_in1 == ex_out_n.alu_in2;
                        break;
                    
                    /* bne */
                    case BNE:
                        br_taken = ex_out_n.alu_in1 != ex_out_n.alu_in2;
                        break;
                    
                    /* blt */
                    case BLT:
                        br_taken = ex_out_n.alu_in1 < ex_out_n.alu_in2;
                        break;
                    
                    /* bge */
                    case BGE:
                        br_taken = ex_out_n.alu_in1 >= ex_out_n.alu_in2;
                        break;   
                }

                // increment the total_branches counter
                total_branches++;
                
                if(branch_prediction_enabled == 1 || branch_prediction_enabled == 2){
                    // compare actual and predicted branches
                    if (br_taken != ex_out_n.br_predicted)
                    {
                        br_mispredicted = 1; // Make sure to set br_mispredicted
                        br_taken ? advance_pc(ex_out_n.br_addr): advance_pc(ex_out_n.inst_addr + 4);
                    } else{
                        correctly_predicted_branches++;
                    }
                    // update branhc prediction structures
                    BTB_update(ex_out_n.inst_addr, ex_out_n.br_addr); // update BTB
                    direction_update(br_taken, ex_out_n.inst_addr); // update BHT
                }
                else {
                    if(br_taken){
                        br_mispredicted = 1; // Make sure to set br_mispredicted
                        advance_pc(ex_out_n.br_addr);
                    } else {
                        correctly_predicted_branches++;
                    }
                }


                break;

            /* lui */
            case LUI:
                we_exe = 1;
                ex_out_n.alu_out = ex_out_n.inst & 0xFFFFF000;
                break;

            /* jal and jalr*/
            case JAL:
            case JALR:
                we_exe = 1;
                break;
        }

        /* Set the dout_exe: jal / jalr should forward the link address, others should formward the alu_out */
        if (ex_out_n.opcode == JAL || ex_out_n.opcode == JALR){
            dout_exe = ex_out_n.link_addr;
        }else{
            dout_exe = ex_out_n.alu_out;
        }
        
        return ex_out_n;
    }

}



/**
 * Execute stage implementation for Load and Stores
 */
struct State execute_ld_st() {
    // read the decode_out state and start processing execute_ld_st stage's functionality 
    ex_ld_st_out_n = decode_out;

    /* Reset relevant pipeline variables */
    we_mem = 0;
    ws_mem = 0;
    dout_mem = 0;
    
    // operate on the ex_ld_st_out struct when dmem is busy executing a load or store multi-cycle instruction
    if (dmem_busy) {
        dmem_busy = 0; // this is needed if this is the last cycle of load or store execution
        
        /* Stall for the correct number of cycles */
        if (++dmem_cycles < dmem_access_cycles){
            dmem_busy = 1;
            if (ex_ld_st_out.opcode == ITYPE_LOAD)
            {
                we_mem = 1;
                ws_mem = ex_ld_st_out.rd; 
            }
            return ex_ld_st_out;
        /* After X cycles,reset cycles */
        } else {
            dmem_cycles = 1;
        }

        // last cycle of a load or store performs the actual memory access
        switch (ex_ld_st_out.opcode) {
            case ITYPE_LOAD:
                ex_ld_st_out.mem_buffer = memory[ex_ld_st_out.mem_addr];
                
                /* Handle the FW for LW */
                we_mem = 1;
                ws_mem = ex_ld_st_out.rd;
                dout_mem = ex_ld_st_out.mem_buffer; 
                break;

            case STYPE:
                memory[ex_ld_st_out.mem_addr] = ex_ld_st_out.mem_buffer;
                break;

            default:
                break;
        }
        return ex_ld_st_out;
    }

    // if dmem_busy is 0 then perform the addr calc for load or store. In case of non load/store, return a NOP
    // ensure this ld/st instruction was allocated to this unit in decode
    else if ((ex_ld_st_out_n.opcode == ITYPE_LOAD || ex_ld_st_out_n.opcode == STYPE) && ex_ld_st_out_n.ld_st_unit == 1) {
        dmem_busy = 1;
        dmem_cycles = 1;

        switch(ex_ld_st_out_n.opcode) {
            /* store, load (both calculate memory address in EX phase) */
            case ITYPE_LOAD:
            we_mem = 1;
            ws_mem = ex_ld_st_out_n.rd;
            case STYPE:
                ex_ld_st_out_n.mem_addr = ex_ld_st_out_n.alu_in1 + ex_ld_st_out_n.alu_in2;
                break;
        }
        return ex_ld_st_out_n;
    }
    else // for non load or store instructions or ld/st not destined to this unit, reset
    {
        dmem_busy = 0;
        return nop;
    }
}


/**
 * Execute stage implementation for second Load and Stores
 */
struct State execute_2nd_ld_st() {
    // read the decode_out state and start processing execute_ld_st stage's functionality 
    ex_ld_st_2_out_n = decode_out;

    /* Reset relevant pipeline variables */
    we_mem2 = 0;
    ws_mem2 = 0;
    dout_mem2 = 0;
    
    // operate on the ex_ld_st_out struct when dmem is busy executing a load or store multi-cycle instruction
    if (dmem_busy2) {
        dmem_busy2 = 0; // this is needed if this is the last cycle of load or store execution
        
        /* Stall for the correct number of cycles */
        if (++dmem_cycles2 < dmem_access_cycles){
            dmem_busy2 = 1;
            if (ex_ld_st_2_out.opcode == ITYPE_LOAD)
            {
                we_mem2 = 1;
                ws_mem2 = ex_ld_st_2_out.rd; 
            }
            return ex_ld_st_2_out;
        /* After X cycles,reset cycles */
        } else {
            dmem_cycles2 = 1;
        }

        // last cycle of a load or store performs the actual memory access
        switch (ex_ld_st_2_out.opcode) {
            case ITYPE_LOAD:
                ex_ld_st_2_out.mem_buffer = memory[ex_ld_st_2_out.mem_addr];
                
                /* Handle the FW for LW */
                we_mem2 = 1;
                ws_mem2 = ex_ld_st_2_out.rd;
                dout_mem2 = ex_ld_st_2_out.mem_buffer; 
                break;

            case STYPE:
                memory[ex_ld_st_2_out.mem_addr] = ex_ld_st_2_out.mem_buffer;
                break;

            default:
                break;
        }
        return ex_ld_st_2_out;
    }

    // if dmem_busy is 0 then perform the addr calc for load or store. In case of non load/store, return a NOP
    // ensure this ld/st instruction was allocated to this unit in decode
    else if ((ex_ld_st_2_out_n.opcode == ITYPE_LOAD || ex_ld_st_2_out_n.opcode == STYPE) && ex_ld_st_2_out_n.ld_st_unit == 2) {
        dmem_busy2 = 1;
        dmem_cycles2 = 1;

        switch(ex_ld_st_2_out_n.opcode) {
            /* store, load (both calculate memory address in EX phase) */
            case ITYPE_LOAD:
            we_mem2 = 1;
            ws_mem2 = ex_ld_st_2_out_n.rd;
            case STYPE:
                ex_ld_st_2_out_n.mem_addr = ex_ld_st_2_out_n.alu_in1 + ex_ld_st_2_out_n.alu_in2;
                break;
        }
        return ex_ld_st_2_out_n;
    }
    else // for non load or store instructions, reset
    {
        dmem_busy2 = 0;
        return nop;
    }
}

/**
 * Writeback stage implementation
 */
void writeback() {


    // read the ex_out and ex_ld_st_out states and start processing writeback stage's functionality 

    if (ooo_enabled == 0)
    {
        //reset pipeline flags
        we_wb = 0;
        ws_wb = 0;
        dout_wb = 0;

        // if instruction is coming from Load/Store FU
        if ((ex_out.inst == nop.inst) && (ex_ld_st_out.inst != nop.inst)) {
            // this tracks that ex_ld_st_out is a valid load or store instruction but dmem_busy indicates that it should be treated as a NOP
            wb_out_n = ex_ld_st_out; 
            if (dmem_busy) {
                wb_out_n.inst   = nop.inst;
                wb_out_n.opcode = ITYPE_ARITH;
                wb_out_n.imm    = 0;
                wb_out_n.rs1    = 0;
                wb_out_n.rd     = 0;
            }
        }
        // if instruction is coming from Non Load/Store FU
        else if ((ex_out.inst != nop.inst) && (ex_ld_st_out.inst == nop.inst)) {
            wb_out_n = ex_out;
        }
        else if ((ex_out.inst != nop.inst) && (ex_ld_st_out.inst != nop.inst)) {
            printf("ERROR: writeback cannot process valid instructions from both execute and execute_ld_st stages\n");
            abort();        
        }
        // if instruction is neither coming from Load/Store FU nor from Non Load/Store FU
        else {
            wb_out_n = ex_out;  //could either be ex_out or ex_ld_st_out
        }

        switch(wb_out_n.opcode){
            // R-type, I-type arithmatic, and lui store a register value to rd //
            case RTYPE:
            case ITYPE_ARITH:
            case LUI:
                we_wb = 1;
                ws_wb = wb_out_n.rd;
                dout_wb = wb_out_n.alu_out;
                registers[wb_out_n.rd] = wb_out_n.alu_out; //All these write ALU output to register
                break;
            // load stores a memory value to rd //
            case ITYPE_LOAD:
                we_wb = 1;
                ws_wb = wb_out_n.rd;
                dout_wb = wb_out_n.mem_buffer; 
                registers[wb_out_n.rd] = wb_out_n.mem_buffer; //LW writes the loaded memory to register
                break;
            // jal / jalr store a link address to rs (only when rs =/= 0) //
            case JAL:
            case JALR:
                ws_wb = wb_out_n.rd;
                dout_exe = wb_out_n.link_addr;
                if (wb_out_n.rd != 0) {
                    we_wb = 1;
                    registers[wb_out_n.rd] = wb_out_n.link_addr; 
                }
                break;
            default:
                break;
        }

    }
    else
    {
        //reset pipeline flags for Non Load Store
        we_wb = 0;
        ws_wb = 0;
        dout_wb = 0;

        //reset pipeline flags for Load Store units
        we_ld_st_wb = 0;
        ws_ld_st_wb = 0;
        dout_ld_st_wb = 0;

        we_ld_st_2_wb = 0;
        ws_ld_st_2_wb = 0;
        dout_ld_st_2_wb = 0;

        // if instruction is coming from Load/Store FU
        if ((ex_out.inst == nop.inst) && (ex_ld_st_out.inst != nop.inst)) {
            // this tracks that ex_ld_st_out is a valid load or store instruction but dmem_busy indicates that it should be treated as a NOP
            
            wb_ld_st_out_n = ex_ld_st_out; 
            if (dmem_busy) {
                wb_ld_st_out_n.inst     = nop.inst;
                wb_ld_st_out_n.opcode   = ITYPE_ARITH;
                wb_ld_st_out_n.imm      = 0;
                wb_ld_st_out_n.rs1      = 0;
                wb_ld_st_out_n.rd       = 0;
            }

            //the Non load store writeback is a nop
            wb_out_n = ex_out;

            // OK: handle ld/st unit 2
            if (ex_ld_st_2_out.inst == nop.inst) {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
            }
            else {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
                if (dmem_busy2) {
                    wb_ld_st_2_out_n.inst     = nop.inst;
                    wb_ld_st_2_out_n.opcode   = ITYPE_ARITH;
                    wb_ld_st_2_out_n.imm      = 0;
                    wb_ld_st_2_out_n.rs1      = 0;
                    wb_ld_st_2_out_n.rd       = 0;
                }
            }

        }
        // if instruction is coming from Non Load/Store FU
        else if ((ex_out.inst != nop.inst) && (ex_ld_st_out.inst == nop.inst)) {
            wb_out_n        = ex_out;
            wb_ld_st_out_n  = ex_ld_st_out; 
            
            // OK: handle ld/st unit 2
            if (ex_ld_st_2_out.inst == nop.inst) {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
            }
            else {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
                if (dmem_busy2) {
                    wb_ld_st_2_out_n.inst     = nop.inst;
                    wb_ld_st_2_out_n.opcode   = ITYPE_ARITH;
                    wb_ld_st_2_out_n.imm      = 0;
                    wb_ld_st_2_out_n.rs1      = 0;
                    wb_ld_st_2_out_n.rd       = 0;
                }
            }
        }
        else if ((ex_out.inst != nop.inst) && (ex_ld_st_out.inst != nop.inst)) {
            if (dmem_busy) {
                wb_ld_st_out_n.inst     = nop.inst;
                wb_ld_st_out_n.opcode   = ITYPE_ARITH;
                wb_ld_st_out_n.imm      = 0;
                wb_ld_st_out_n.rs1      = 0;
                wb_ld_st_out_n.rd       = 0;
            }
            else
            {
                wb_ld_st_out_n = ex_ld_st_out; 
            }
            wb_out_n = ex_out;

            // OK: handle ld/st unit 2
            if (ex_ld_st_2_out.inst == nop.inst) {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
            }
            else {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
                if (dmem_busy2) {
                    wb_ld_st_2_out_n.inst     = nop.inst;
                    wb_ld_st_2_out_n.opcode   = ITYPE_ARITH;
                    wb_ld_st_2_out_n.imm      = 0;
                    wb_ld_st_2_out_n.rs1      = 0;
                    wb_ld_st_2_out_n.rd       = 0;
                }
            }

        }
        // if instruction is neither coming from Load/Store FU nor from Non Load/Store FU
        else {

            //since they are both nop
            wb_out_n = ex_out;
            wb_ld_st_out_n = ex_ld_st_out; 

            // OK: handle ld/st unit 2
            if (ex_ld_st_2_out.inst == nop.inst) {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
            }
            else {
                wb_ld_st_2_out_n = ex_ld_st_2_out; 
                if (dmem_busy2) {
                    wb_ld_st_2_out_n.inst     = nop.inst;
                    wb_ld_st_2_out_n.opcode   = ITYPE_ARITH;
                    wb_ld_st_2_out_n.imm      = 0;
                    wb_ld_st_2_out_n.rs1      = 0;
                    wb_ld_st_2_out_n.rd       = 0;
                }
            }
        }

        switch(wb_out_n.opcode)
        {
            // R-type, I-type arithmatic, and lui store a register value to rd //
            case RTYPE:
            case ITYPE_ARITH:
            case LUI:
                we_wb = 1;
                ws_wb = wb_out_n.rd;
                dout_wb = wb_out_n.alu_out;
                registers[wb_out_n.rd] = wb_out_n.alu_out; //All these write ALU output to register
                break;
            // jal / jalr store a link address to rs (only when rs =/= 0) //
            case JAL:
            case JALR:
                ws_wb = wb_out_n.rd;
                dout_exe = wb_out_n.link_addr;
                if (wb_out_n.rd != 0) {
                    we_wb = 1;
                    registers[wb_out_n.rd] = wb_out_n.link_addr; 
                }
                break;
            default:
                break;
        }

        switch(wb_ld_st_out_n.opcode){
            // load stores a memory value to rd //
            case ITYPE_LOAD:
                we_ld_st_wb = 1;
                ws_ld_st_wb = wb_ld_st_out_n.rd;
                dout_ld_st_wb = wb_ld_st_out_n.mem_buffer; 
                registers[wb_ld_st_out_n.rd] = wb_ld_st_out_n.mem_buffer; //LW writes the loaded memory to register
                break;
            default:
                break;
        }

        // OK: possible Load from unit 2
        switch(wb_ld_st_2_out_n.opcode){
            // load stores a memory value to rd //
            case ITYPE_LOAD:
                we_ld_st_2_wb = 1;
                ws_ld_st_2_wb = wb_ld_st_2_out_n.rd;
                dout_ld_st_2_wb = wb_ld_st_2_out_n.mem_buffer; 
                registers[wb_ld_st_2_out_n.rd] = wb_ld_st_2_out_n.mem_buffer;
                break;
            default:
                break;
        }

    }

    
}






/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}


/* BRANCH PREDICTION FUNCTIONS*/

/**
 * Branch Target Buffer Hit Lookup:
*/
unsigned int BTB_lookup(unsigned int inst_addr){
   /*
    Checks if the instruction is in the Branch Target Buffer (BTB).
    Logic:

    - Compute the index using bits from the instruction address.
    - Verify if the inst_addr matches the BTB entry and the valid bit is set.
    - Return 1 if found (hit), otherwise 0 (miss).
    

   BTB_lookup() takes an instruction’s address as the input. The input instruction address determines
if the indexed BTB entry’s instruction address matches the input instruction address, and if the
entry is valid, 1 is returned (BTB hit). Otherwise, it must return 0 (BTB miss).

*/



   uint32_t index = (inst_addr >> 2) & 0x1F; // extracting the bits from 2 - 6
   if(btb[index].valid && btb[index].inst_addr == inst_addr){
        return 1; // BTB Hit
   }
    return 0; // BTB miss

}


/**
 * Branch target buffer target address lookup
*/
unsigned int BTB_target(unsigned int inst_addr){
    
    /*Retrieves the branch target address from the BTB for a given instruction.
    Logic:
    - Similar indexing as BTB_lookup.
    - Return the target address.*/

    uint32_t index = (inst_addr >> 2) & 0x1F; // extracting biuts 2-6
    return btb[index].branch_target; // return target address
}

/**
 * Branch target buffer and direction predictor update
 */
void BTB_update(unsigned int inst_addr, unsigned int branch_target){
    /*
    Updates the BTB with the latest instruction and branch target address.

    Logic:
    - Compute the index from the instruction address.
    - Update the BTB entry with the inst_addr, branch_target, and set valid to 1.*/

    uint32_t index = (inst_addr >> 2) & 0x1F; // extracting bits 2-6
    btb[index].inst_addr = inst_addr;
    btb[index].branch_target = branch_target;
    btb[index].valid = 1; // set entry as valid
}

/** 
 * Direction predictor lookup
*/
unsigned int predict_direction(unsigned int inst_addr){

    /** Predicts branch direction based on BHT.
    
    Logic:
    - Use the instruction address to index into the BHT (1-level) or combine with BHSR (2-level).
    - Interpret the 2-bit state to decide "Taken" or "Not Taken".
    
    For a 1-level predictor the function takes an instruction address to index into
    the BHT, whereas for a 2-level predictor BHSR is used to index into the BHT and the corresponding
    prediction bits are checked as shown in 1.1. If the bits indicate “T” or “TN”, then this function returns
    ‘1’ (for predict branch taken). Otherwise, this function returns ‘0’ (for branch not taken).
    */

    uint32_t index;
    if(branch_prediction_enabled == 1){
        // 1-level predictor
        index = (inst_addr >> 2) & 0x1F; // extract bits 2-6
    } else if(branch_prediction_enabled == 2){
        // 2-level predictor
        index = ((inst_addr >> 2) & 0x1F) ^ bhsr; // extract bits 2-6 and XOR with BHSR
    } else{
        return 0; // no prediction
    }

    uint32_t bht_state = bht[index];

    // strongly/weakly taken states
    if(bht_state == 2 || bht_state == 3){
        return 1; // predict as taken
    } 
    return 0; // predict as not taken for 0, 1 states - weakly/strongly not taken
    
}


/* Direction Predictor Update */
void direction_update(unsigned int direction,unsigned int inst_addr){
    
    /** Updates the BHT and BHSR based on the actual branch outcome.
    
    Logic:
    - Update the 2-bit state in BHT based on the branch result (Taken or Not Taken).
    - Shift the result into the BHSR.
    
    The function takes the instruction address and the branch direction as the
    input (‘1’ for taken, ‘0’ for not taken). For 1-level predictor the instruction address is used to index
    into the BHT for update, whereas for 2-level predictor the BHSR is used to determine which BHT
    entry is being updated. The state machine logic performs state transitions based on the actual
    branch direction using 1.1.
    For a 2-level predictor, the BHSR is also updated after updating the BHT. The input direction bit is
    shifted into the BHSR by shifting the BHSR to the left by 1 and bitwise ORing the least significant
    bit with the direction bit. Remember that only bits 4 to 0 are used to record branch
    history, as the BHT has only 32 entries. Upper bits beyond bit 4 must be cleared to
    ‘0’.
    
    
    */

//    uint32_t index = (inst_addr >> 2) & 0x1F; // extract bits 2 to 6
//    uint32_t bht_state = bht[index];

//    // state transiitions
//    if(direction) { // actual branch taken
//     if(bht_state < 3) bht_state ++;
//    } else{ // actual is not taken
//     if(bht_state > 0) bht_state--;
//    }
//    bht[index] = bht_state; // update the BHT state

    uint32_t index;
    if(branch_prediction_enabled == 1){
        // 1-level predictor
        index = (inst_addr >> 2) & 0x1F; // extract bits 2-6
    } else if(branch_prediction_enabled == 2){
        // 2-level predictor
        // index = ((inst_addr >> 2) ^ bhsr) & 0x1F; 
        index = ((inst_addr >> 2) & 0x1F) ^ bhsr; // extract bits 2-6 and XOR with BHSR
    } else{
        return; // no prediction
    }

    uint32_t bht_state = bht[index];

    // state transitions
    if(direction) { // actual branch taken
        if(bht_state < 3) bht_state ++;
    } else{ // actual is not taken
        if(bht_state > 0) bht_state--;
    }
    bht[index] = bht_state; // update the BHT state'

    // update the BHSR for 2-level predictor
    if(branch_prediction_enabled == 2){
               bhsr = ((bhsr << 1) | direction) & 0x1F; // shift left and OR with direction, then mask to 5 bits
    }



}


/* DATA CACHE FUNCTIONS */

/**
 * Data cache lookup
 */

unsigned int dcache_lookup_4_way(unsigned int addr_mem) {
/*
    Determines if a memory address is in the cache (hit/miss).
    Logic:
    - Parse the memory address into index, tag, and offset bits.
    - For the respective cache (DM, 2-way, or 4-way):
        - Check if the valid bit is set and the tag matches.
        - Return the way of the hit, or -1 for a miss.
*/

    uint32_t index = (addr_mem >> 2) & 0x3; // extracting index bits (4 sets)
    uint32_t tag = addr_mem >> 8; // extracting bits 7-31


    // check if the valid bit is set and the tag matches
    for(int i = 0; i < 4; i++){
        if(dcache_4_way[index].block[i].valid && dcache_4_way[index].block[i].tag == tag){
            return i; // cache hit
        }
    }
    return -1; // cache miss

}


/**
 * Data cache update
 */
void dcache_update_4_way(unsigned int addr_mem, int line) {
    
    uint32_t index = (addr_mem >> 2) & 0x3; // extracting index bits 2-3
    uint32_t tag = addr_mem >> 8; // extracting tag bits 7-31

    if(line >= 0){
        // cache hit: update the cache block
        dcache_4_way[index].block[line].tag = tag;
        dcache_4_way[index].block[line].valid = 1; // set the valid bit
    } else{
        // cache miss: Find LRU block and update
        uint8_t lru_bits = dcache_4_way[index].lru_tree;
        int lru_block = (lru_bits == 0b00) ? 0 : (lru_bits == 0b01) ? 1 : (lru_bits == 0b10) ? 2 : 3;

        dcache_4_way[index].block[lru_block].tag = tag;
        dcache_4_way[index].block[lru_block].valid = 1; // set the valid bit

        // update the LRU tree for the next cycle
        dcache_4_way[index].lru_tree = (lru_block + 1) % 4;
    }
    
}


unsigned int dcache_lookup_2_way(unsigned int addr_mem) {
    uint32_t index = (addr_mem >> 4) & 0x7; // extract index bits (16 sets)
    uint32_t tag = addr_mem >> 8; // extract tag bits (24 bits)

    // check if the valid bit is set and the tag matches
    for(int i = 0; i < 2; i++){
        if(dcache_2_way[index].block[i].valid && dcache_2_way[index].block[i].tag == tag){
            return i; // hit in way i
        }
    }
    return -1; // miss
   
}


/**
 * Data cache update
 */
void dcache_update_2_way(unsigned int addr_mem, int line) {

  uint32_t index = (addr_mem >> 4) & 0x7; // extracting bits 4-6
    uint32_t tag = addr_mem >> 8; // extracting bits 7-31

    if(line >= 0){
        // cache hit: update the cache block
        dcache_2_way[index].block[line].tag = tag;
        dcache_2_way[index].block[line].valid = 1; // set the valid bit
    } else{
        //cache miss: Find LRU block and update
        int lru = (dcache_2_way[index].lru_tree == 0) ? 0 : 1;
        dcache_2_way[index].block[lru].tag = tag;
        dcache_2_way[index].block[lru].valid = 1; // set the valid bit

        // update the LRU tree
        dcache_2_way[index].lru_tree = !dcache_2_way[index].lru_tree;
    }
    
}



// One way data cache implementation
unsigned int dcache_lookup_DM(unsigned int addr_mem) {

    uint32_t index = (addr_mem >> 4) & 0xF; // extract index bits (16 sets)
    uint32_t tag = addr_mem >> 8; // extract tag bits (24 bits)

    // check if the valid bit is set and the tag matches
    if(dcache_DM[index].block[0].valid && dcache_DM[index].block[0].tag == tag){
        return 0; // hit in way 0
    }
    return -1; // miss
    

}


/**
 * Data cache update
 */
void dcache_update_DM(unsigned int addr_mem) {
    
    uint32_t index = (addr_mem >> 2) & 0x1F; // extracting bits 2-6
    uint32_t tag = addr_mem >> 8; // extracting bits 7-31

    // update the cache block
    dcache_DM[index].block[0].tag = tag;
    dcache_DM[index].block[0].valid = 1; // set the valid bit
 


}