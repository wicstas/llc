#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
        for(int i = 0; i < 10; i++){
            print(i);
        }

        int i = 0;
        while(i++ < 20){
            if(i > 15){
                print(i);
            }
        }
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}