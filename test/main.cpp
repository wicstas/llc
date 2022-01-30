#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
       float bear_weight = 240.0f;
       float salmon_weight = 1.3f;
       float bear_weight_after_dinner = add(bear_weight, salmon_weight);
       print(bear_weight_after_dinner);
    )";

    Compiler compiler;
    compiler.register_function("print", print<float>);
    compiler.register_function(
        "add", +[](float a, float b) { return a + b; });

    Program program = compiler.compile(source);
    program.run();

    return 0;
}