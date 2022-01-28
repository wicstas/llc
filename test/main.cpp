#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       int fibonacci(int v0,int v1){
            int v2 = v0 + v1;
            if(v0 < 100){
                print(v0);
                fibonacci(v1, v2);
            }
       }
    //    fibonacci(0, 1);

       int is_prime(int number){
           for(int i = 2;i < number;i++){
               if((number / i * i) == number){
                   return 0;
               }
           }
           return 1;
       }

       for(int i = 0; i < 100; i++){
           if(is_prime(i)){
                print(i);
           }
       }
       
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}