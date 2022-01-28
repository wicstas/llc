#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       void fibonacci(int v0,int v1){
            int v2 = v0 + v1;
            if(v2 < 100){
                print(v0);
                fibonacci(v1, v2);
            }
       }

       fibonacci(0, 1);
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}