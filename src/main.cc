#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#include "core.hpp"
#include "definitions.h"

void usage()
{
    std::cout << "Usage: ./<program> <binary_filename>\n";
}

int main(int argc, char** argv)
{
    if(argc!=2){
        usage();
    }else{
        disable_buffered_input();
        std::fstream binary_in(argv[1],std::ios::binary|std::ios::ate|std::ios::in);
        std::streamsize size = binary_in.tellg();
        binary_in.seekg(0, std::ios::beg);
        std::vector<uint8_t> binary(MEM_SIZE,0);
        binary_in.read(reinterpret_cast<char*>(binary.data()), size);
        binary_in.close();
        // init platform
        auto cpu = Core(binary);
        
        while(true){
            try{
                auto instruction = cpu.fetch();
                if(!instruction.has_value())
                    break;
                cpu.execute(instruction.value());
                cpu.check_and_handle_interrupts();
            }
            catch(std::runtime_error& err){
                std::cout << err.what() << '\n';
                break;
            }
            catch(LoadAccessFault& lf){
                cpu.get_exception_handler().handle(cpu, 0, 5, lf.get_addr());
            }
            catch(StoreAccessFault& sf){
                cpu.get_exception_handler().handle(cpu, 0, 7, sf.get_addr());
            }
            catch(ContinueExec& control){
                // 这个异常专用于转移控制流
            }
        }
        cpu.dump_registers();
    }
    return 0;
}