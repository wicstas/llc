#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       int* value = new int[10];
       for(int i = 0;i < 10;i++)
           value[i] = i;
       for(int i = 0;i < 10;i++)
        print(value[i]);
    )";

    Compiler compiler;
    Program program = compiler.compile(source);
    program->run(*program);

    return 0;
}