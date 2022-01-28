#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       struct Planet{
           float mass;
           float size;
           int age;
       };

       Planet init_planet(){
            Planet planet;
            planet.mass = 1000.0f;
            planet.size = 10.7f;
            planet.age = 31.2f;
            return planet;
       }

       print(init_planet());
    )";

    Compiler compiler;
    auto program = compiler.compile(source);
    program->run(*program);

    return 0;
}