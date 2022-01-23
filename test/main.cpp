#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
        int i = 0;
        while(i++ < 10){
            if(i == 5){
                print(100);
            }else if(i == 6){
                print(200);
            }else{
                print(i);
            }
        }
    )";

    Compiler compiler;
    compiler.compile(source);
    compiler.execute();

    return 0;
}