#include "core.hpp"

// instruction runner
void Core::exec_load(uint32_t instruction)
{
    uint32_t offset = extract_bits(instruction, 31, 20);
    uint32_t signed_offset = extend_to_u32(offset, 12);
    uint32_t rs1    = extract_bits(instruction, 19,15);
    uint32_t func3  = extract_bits(instruction, 14,12);
    uint32_t rd     = extract_bits(instruction, 11,7);
    uint32_t addr   = signed_offset+regfile[rs1];
    switch(func3){
        case 0b000:
            regfile[rd] = extend_to_u32(cpu_io.load(addr, 8), 8);
            break;
        case 0b001:
            regfile[rd] = extend_to_u32(cpu_io.load(addr, 16), 16);
            break;
        case 0b010:
            regfile[rd] = cpu_io.load(addr, 32);
            break;
        case 0b100:
            regfile[rd] = cpu_io.load(addr, 8);
            break;
        case 0b101:
            regfile[rd] = cpu_io.load(addr, 16);
            break;
        default:
            exec_illegal(instruction);
            return;
    }
    pc += 4;
}
void Core::exec_alg_logic_imm(uint32_t instruction)
{
    uint32_t imm = extract_bits(instruction, 31, 20);
    uint32_t signed_imm = extend_to_u32(imm, 12);
    uint32_t rs1   = extract_bits(instruction, 19,15);
    uint32_t func3 = extract_bits(instruction, 14,12);
    uint32_t rd    = extract_bits(instruction, 11,7);
    uint32_t shamt = extract_bits(instruction, 24, 20);
    uint32_t shift = extract_bits(instruction, 31, 25);
    switch(func3){
        case 0b000:
            regfile[rd] = regfile[rs1] + signed_imm;
            break;
        case 0b001:
            if(shamt>32||shift!=0){
                exec_illegal(instruction);
                return;
            }
            else
                regfile[rd] = regfile[rs1] << shamt;
            break;
        case 0b010:
            regfile[rd] = regfile[rs1] < signed_imm ? 1 : 0;
            break;
        case 0b011:
            regfile[rd] = regfile[rs1] < imm ? 1 : 0;
            break;
        case 0b100:
            regfile[rd] = regfile[rs1] ^ signed_imm;
            break;
        case 0b101:{
            uint32_t sign = regfile[rs1] >> 31;
            if(shamt>32){
                exec_illegal(instruction);
                return;
            }
            if(shift==0 || shift==0b100000 && sign==0)
                regfile[rd] = regfile[rs1] >> shamt;
            else if(shift==0b100000 && sign==1)
                regfile[rd] = static_cast<int32_t>(regfile[rs1])>>shamt;
            else{
                exec_illegal(instruction);
                return;
            }
            break;
        }
        case 0b110:
            regfile[rd] = regfile[rs1] | signed_imm;
            break;
        case 0b111:
            regfile[rd] = regfile[rs1] & signed_imm;
            break;
    }
    pc += 4;
}
void Core::exec_auipc(uint32_t instruction)
{
    uint32_t integer = extract_bits(instruction, 31, 12);
    uint32_t rd = extract_bits(instruction, 11, 7);
    regfile[rd] = (integer << 12) + pc;
    pc += 4;
}
void Core::exec_store(uint32_t instruction)
{
    uint32_t offset = extract_bits(instruction, 31, 25)<<5 |
                      extract_bits(instruction, 11,7);
    uint32_t signed_offset = extend_to_u32(offset, 12);
    uint32_t rs2    = extract_bits(instruction, 24,20);
    uint32_t rs1    = extract_bits(instruction, 19,15);
    uint32_t func3  = extract_bits(instruction, 14,12);
    uint32_t addr   = regfile[rs1]+signed_offset;
    switch(func3){
        case 0b000:
            cpu_io.store(extract_bits(regfile[rs2], 7, 0), addr, 8);
            reserved.erase(addr);
            break;
        case 0b001:
            cpu_io.store(extract_bits(regfile[rs2], 15, 0), addr, 16);
            reserved.erase(addr);
            break;
        case 0b010:
            cpu_io.store(regfile[rs2], addr, 32);
            reserved.erase(addr);
            break;
        default:
            exec_illegal(instruction);
            return;
    }
    pc += 4;
}
void Core::exec_alg_logic(uint32_t instruction)
{
    uint32_t func7 = extract_bits(instruction, 31, 25);
    uint32_t rs2   = extract_bits(instruction, 24,20);
    uint32_t rs1   = extract_bits(instruction, 19,15);
    uint32_t func3 = extract_bits(instruction, 14,12);
    uint32_t rd    = extract_bits(instruction, 11,7);
    if(func7==1){
        int64_t op1   = static_cast<int64_t>(static_cast<int32_t>(regfile[rs1]));
        int64_t op2   = static_cast<int64_t>(static_cast<int32_t>(regfile[rs2]));
        uint64_t uop1 = static_cast<uint64_t>(regfile[rs1]);
        uint64_t uop2 = static_cast<uint64_t>(regfile[rs2]);
        switch(func3){
            case 0b000:
                regfile[rd] = static_cast<uint32_t>(op1*op2);
                break;
            case 0b001:
                regfile[rd] = static_cast<uint32_t>((op1*op2)>>32);
                break;
            case 0b010:
                regfile[rd] = static_cast<uint32_t>(op1*uop2);
                break;
            case 0b011:
                regfile[rd] = static_cast<uint32_t>(uop1*uop2);
                break;
            case 0b100:
                regfile[rd] = static_cast<int32_t>(regfile[rs1]) / static_cast<int32_t>(regfile[rs2]);
                break;
            case 0b101:
                regfile[rd] = regfile[rs1] / regfile[rs2];
                break;
            case 0b110:
                regfile[rd] = static_cast<int32_t>(regfile[rs1]) % static_cast<int32_t>(regfile[rs2]);
                break;
            case 0b111:
                regfile[rd] = regfile[rs1] % regfile[rs2];
                break;
        }
        pc += 4;
        return;
    }
    switch(func3){
        case 0b000:
            if(func7==0)
                regfile[rd] = regfile[rs1] + regfile[rs2];
            else if(func7==0b100000)
                regfile[rd] = regfile[rs1] - regfile[rs2];
            else{
                exec_illegal(instruction);
                return;
            }
            break;
        case 0b001:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] << (regfile[rs2]&0b011111);
            break;
        case 0b010:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] < static_cast<int32_t>(regfile[rs2]) ? 1 : 0;
            break;
        case 0b011:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] < regfile[rs2] ? 1 : 0;
            break;
        case 0b100:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] ^ regfile[rs2];
            break;
        case 0b101:
            if(func7==0)
                regfile[rd] = regfile[rs1] >> (regfile[rs2]&0b011111);
            else if(func7==0b100000)
                regfile[rd] = static_cast<int32_t>(regfile[rs1]) >> (regfile[rs2]&0b011111);              
            else{
                exec_illegal(instruction);
                return;
            }
            break;
        case 0b110:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] | regfile[rs2];
            break;
        case 0b111:
            if(func7!=0){
                exec_illegal(instruction);
                return;
            }
            regfile[rd] = regfile[rs1] & regfile[rs2];
            break;
    }
    pc += 4;
}
void Core::exec_lui(uint32_t instruction)
{
    uint32_t integer = extract_bits(instruction, 31, 12);
    uint32_t rd = extract_bits(instruction, 11, 7);
    regfile[rd] = integer << 12;
    pc += 4;
}
void Core::exec_branch(uint32_t instruction)
{
    uint32_t offset = extract_bits(instruction, 31, 31)<<12 |
                      extract_bits(instruction, 7, 7)<<11 |
                      extract_bits(instruction, 30, 25)<<5 |
                      extract_bits(instruction, 11,8)<<1;
    uint32_t signed_offset = extend_to_u32(offset, 12);
    uint32_t rs2    = extract_bits(instruction, 24,20);
    uint32_t rs1    = extract_bits(instruction, 19,15);
    uint32_t func3  = extract_bits(instruction, 14,12);
    switch(func3){
        case 0b000:
            if(regfile[rs1]==regfile[rs2])
                pc += signed_offset;
            else
                pc += 4;
            break;
        case 0b001:
            if(regfile[rs1]!=regfile[rs2])
                pc += signed_offset;
            else
                pc += 4;
            break;
        case 0b100:
            if(static_cast<int32_t>(regfile[rs1])<static_cast<int32_t>(regfile[rs2]))
                pc+=signed_offset;
            else
                pc += 4;
            break;
        case 0b101:
            if(static_cast<int32_t>(regfile[rs1])>=static_cast<int32_t>(regfile[rs2]))
                pc+=signed_offset;
            else
                pc += 4;
            break;
        case 0b110:
            if(regfile[rs1]<regfile[rs2])
                pc+=signed_offset;
            else
                pc += 4;
            break;
        case 0b111:
            if(regfile[rs1]>=regfile[rs2])
                pc+=signed_offset;
            else
                pc += 4;
            break;
        default:
            exec_illegal(instruction);
            return;
    }
}
void Core::exec_jalr(uint32_t instruction)
{
    uint32_t offset = extract_bits(instruction, 31, 20);
    uint32_t signed_offset = extend_to_u32(offset, 12);
    uint32_t rs1 = extract_bits(instruction, 19, 15);
    uint32_t func3 = extract_bits(instruction, 14, 12);
    uint32_t rd = extract_bits(instruction, 11, 7);
    if(func3!=0b000){
        exec_illegal(instruction);
        return;
    }
    regfile[rd] = pc+4;
    pc = (regfile[rs1]+signed_offset)&~1;    
}
void Core::exec_jal(uint32_t instruction)
{
    uint32_t offset = extract_bits(instruction, 31, 31)<<20|
                      extract_bits(instruction, 19, 12)<<12|
                      extract_bits(instruction, 20, 20)<<11|
                      extract_bits(instruction, 30, 21)<<1;
    uint32_t signed_offset = extend_to_u32(offset, 20);
    uint32_t rd = extract_bits(instruction, 11, 7);
    regfile[rd] = pc+4;
    pc += signed_offset;
}
void Core::exec_fence(uint32_t instruction)
{
    uint32_t func4 = extract_bits(instruction, 31, 28);
    uint32_t pred = extract_bits(instruction, 27, 24);
    uint32_t succ = extract_bits(instruction, 23, 20);
    uint32_t unused = extract_bits(instruction, 19, 15);
    uint32_t func3 = extract_bits(instruction, 14, 12);
    uint32_t unused2 = extract_bits(instruction, 11, 7);
    if(unused2||unused||func4){
        exec_illegal(instruction);
        return;
    }
    if(func3&&(pred||succ)){
        exec_illegal(instruction);
        return;
    }
    // unnecessary

    pc += 4;
}
void Core::exec_atomic(uint32_t instruction)
{
    uint32_t opcode = extract_bits(instruction, 31, 27);
    bool aq = extract_bits(instruction, 26, 26);
    bool rl = extract_bits(instruction, 25, 25);
    uint32_t rs2 = extract_bits(instruction, 24, 20);
    uint32_t rs1 = extract_bits(instruction, 19, 15);
    uint32_t func3 = extract_bits(instruction, 14, 12);
    uint32_t rd = extract_bits(instruction, 11, 7);
    if(func3!=0b010){
        exec_illegal(instruction);
        return;
    }
    uint32_t addr = regfile[rs1];
    if((addr&0b11)!=0)
        exception_handler.handle(*this, 0, 6, addr);
    uint32_t temp = 0;
    try{
        temp = cpu_io.load(addr, 32);
    }catch(std::exception& e){
        return;
    }
    try{
        cpu_io.store(temp, addr, 32);
    }catch(std::exception& e){
        return;
    }
    switch (opcode) {
        case 0b00010:   // lr.w
            if(rs2!=0){
                exec_illegal(instruction);
                return;
            }
            reserved.insert(addr);
            regfile[rd] = temp;
            break;
        case 0b00011:   // sc.w
            if(reserved.count(addr)){
                cpu_io.store(regfile[rs2],addr,32);
                regfile[rd]=0;
            }else{
                regfile[rd]=~0;
            }
            reserved.clear();
            break;
        case 0b00000:  // amoadd
            cpu_io.store(temp+regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b00001:  // amoswap
            cpu_io.store(regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b00100:  // amoxor
            cpu_io.store(temp^regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b01100:  // amoand
            cpu_io.store(temp&regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b01000:  // amoor
            cpu_io.store(temp|regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b10000:  // amomin
            cpu_io.store(static_cast<int32_t>(temp)<=static_cast<int32_t>(regfile[rs2])?temp:regfile[rs2],
                         addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b10100:  // amomax
            cpu_io.store(static_cast<int32_t>(temp)>static_cast<int32_t>(regfile[rs2])?temp:regfile[rs2],
                         addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);        
            break;
        case 0b11000:  // amominu
            cpu_io.store(temp<=regfile[rs2]?temp:regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        case 0b11100:  // amomaxu               
            cpu_io.store(temp>regfile[rs2]?temp:regfile[rs2],addr,32);
            regfile[rd]=temp;
            reserved.erase(addr);
            break;
        default:
            exec_illegal(instruction);
            return;
    }
    pc+=4;
    return;
}
void Core::exec_system(uint32_t instruction)
{
    uint32_t csr_addr = extract_bits(instruction, 31, 20);
    uint32_t func12   = csr_addr;
    uint32_t rs1      = extract_bits(instruction, 19, 15);
    uint32_t zimm     = rs1;
    uint32_t func3    = extract_bits(instruction, 14, 12);
    uint32_t rd       = extract_bits(instruction, 11, 7);
    if(func3!=0){   
        // CSR
        switch(func3){
            case 0b001:
                if( min_priv(csr_addr)>mode || is_readonly(csr_addr)){
                    exec_illegal(instruction);
                    return;
                }
                regfile[rd] = csr.read(csr_addr);
                csr.write(csr_addr, regfile[rs1]);
                break;
            case 0b010:{
                // csrrs
                if( min_priv(csr_addr)>mode || (rs1!=0&&is_readonly(csr_addr))){
                    exec_illegal(instruction);
                    return;
                }
                uint32_t t = csr.read(csr_addr);
                csr.write(csr_addr, regfile[rs1]|t);
                regfile[rd] = t;
                break;
            }
            case 0b011:{
                if( min_priv(csr_addr)>mode || is_readonly(csr_addr)){
                    exec_illegal(instruction);
                    return;
                }
                uint32_t t = csr.read(csr_addr);
                csr.write(csr_addr, ~regfile[rs1]&t);
                regfile[rd] = t;
                break;
            }
            case 0b110:{
                if( min_priv(csr_addr)>mode || is_readonly(csr_addr)){
                    exec_illegal(instruction);
                    return;
                }
                uint32_t t = csr.read(csr_addr);
                csr.write(csr_addr, zimm|t);
                regfile[rd] = t;
                break;
            }
            case 0b111:{
                if( min_priv(csr_addr)>mode || is_readonly(csr_addr)){
                    exec_illegal(instruction);
                    return;
                }
                uint32_t t = csr.read(csr_addr);
                csr.write(csr_addr, ~zimm&t);
                regfile[rd] = t;
                break;
            }
            default:
                exec_illegal(instruction);
                return;
        }
        pc += 4;
        return;
    }
    if(func12==0b0011'0000'0010){ // mret
        if(handling_timer_int){
            handling_timer_int = 0;
            cpu_io.clear_clint_int();
        }
        if(current_irq){
            cpu_io.store(current_irq, PLIC_CLAIM,32);
            current_irq = 0;
        }
        uint32_t mstat = csr.read(mstatus);
        // 恢复权限级别
        mode = (mstat&MASK_MPP) >> 11;
        // MIE 设置为 MPIE
        uint32_t mpie = (mstat&MASK_MPIE) >> 7;
        mstat = (mstat&~MASK_MIE)|(mpie<<3);
        // MPIE 设置为 1
        mstat |= MASK_MPIE;
        // 设置MPP
        mstat &= ~MASK_MPP;
        csr.write(mstatus, mstat);
        // 设置PC
        auto last = pc;
        pc = csr.read(mepc) & ~0b11; 
        return;
    }
    if(func12==0b0001'0000'0101){   // wfi
        while( (csr.read(mstatus)&MASK_MIE) && !(csr.read(mie)&csr.read(mip)));
        pc += 4;
        return;
    }
    if((func12>>5)==0b0001001){   // sfence.vma: do nothing
        pc += 4;
        return;
    }
    if(rs1!=0||func3!=0||rd!=0||func12!=0&&func12!=1){
        exec_illegal(instruction);
        return;
    }
    if(func12==1){  
        // ebreak
        exception_handler.handle(*this, 0, 3, instruction);
        return;
    }else{  
        // ecall
        if(mode==USER_MODE)
            exception_handler.handle(*this, 0, 8, instruction);
        else if(mode==MACHINE_MODE)
            exception_handler.handle(*this, 0,11, instruction);
        return;
    }
    exec_illegal(instruction);
}
void Core::exec_illegal(uint32_t instruction)
{
    exception_handler.handle(*this, 0, 2, instruction);
}