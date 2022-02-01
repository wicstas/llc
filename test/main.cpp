#include <llc/compiler.h>

using namespace llc;

struct Bear {
    float weight;
    float attack;
    float defence;
};

int main() {
    std::string source = R"(
        Bear bear;
        float size = 1.0f;
        printf(size);
    )";

    Compiler compiler;

    compiler.bind<Bear>("Bear")
        .bind("weight", &Bear::weight)
        .bind("attack", &Bear::attack)
        .bind("defence", &Bear::defence);

    compiler.bind("printf", print<float>);

    Program program = compiler.compile(source);
    program.run();

    return 0;
}