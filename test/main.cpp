#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
        for(int i = 0; i < 10; i++){
            print(i);
        }

        i = 0;
        while(i++ < 10){
            if(i < 3){
                print(i * 10);
            }else if(i < 6){
                print(i * 100);
            }else{
                print(i * 1000);
            }
        }
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}