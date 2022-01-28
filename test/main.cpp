#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       for(int i = 0;i < 10;i++){
            if(i < 2)
                print(i * 100);
            else if(i < 5)
                print(i ** 10);
            else
                print(i);
        }
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}