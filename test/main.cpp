#include <llc/compiler.h>

using namespace llc;

struct Bear {
    float weight;
    float attack;
    float defence;
};

void print_bear(Bear bear) {
    print("A bear:");
    print("  weight:", bear.weight);
    print("  attack:", bear.attack);
    print("  defence:", bear.defence);
}

int main() {
    std::string source = R"(
        Bear bear;
        bear.weight = 200.0f;
        bear.attack = 10.0f;
        bear.defence = 5.0f;
        print_bear(bear);
    )";

    Compiler compiler;

    compiler.bind<Bear>("Bear")
        .bind("weight", &Bear::weight)
        .bind("attack", &Bear::attack)
        .bind("defence", &Bear::defence);

    compiler.bind("printf", print<float>);
    compiler.bind("print_bear", print_bear);

    Program program = compiler.compile(source);
    program.run();

    return 0;
}