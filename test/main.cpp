#include <llc/compiler.h>

using namespace llc;

struct Bear {
    float weight;
    float attack;
    float defence;
};

void print_bear_from_outside(Bear bear) {
    print("Bear from outside:", "\n  weight:", bear.weight, "\n  attack:", bear.attack,
          "\n  defence:", bear.defence);
}

int main() {
    std::string source = R"(
        void print_bear_from_inside(Bear bear){
            prints("Bear from inside:");
            printsf("  weight:",bear.weight);
            printsf("  attack:",bear.attack);
            printsf("  defence:",bear.defence);
        };

       Bear bear;
       bear.weight = 240.0f;
       bear.attack = 32.0f;
       bear.defence = 60.0f;

       print_bear_from_outside(bear);
       print_bear_from_inside(bear);
    )";

    Compiler compiler;

    compiler.bind<Bear>("Bear")
        .bind("weight", &Bear::weight)
        .bind("attack", &Bear::attack)
        .bind("defence", &Bear::defence);

    compiler.bind("printf", print<float>);
    compiler.bind("prints", print<std::string>);
    compiler.bind("printsf", print<std::string, float>);
    compiler.bind("print_bear_from_outside", print_bear_from_outside);

    Program program = compiler.compile(source);
    program.run();

    return 0;
}