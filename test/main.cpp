#include <llc/compiler.h>

using namespace llc;

struct Bear {
    std::string name;
    float weight;
    float attack;
    float defence;
};

struct Population {
    Bear bear;
};

void print_population(Population population) {
    print("Population:");

    Bear bear = population.bear;
    print("  Bear: ", bear.name);
    print("    weight : ", bear.weight);
    print("    attack : ", bear.attack);
    print("    defence: ", bear.defence);
}

int main() {
    std::string source = R"(
        Population population;

        Bear bear;
        bear.name = "The chosen one";
        bear.weight = 200.0f;
        bear.attack = 12.0f;
        bear.defence = 8.0f;
        population.bear = bear;

        print_population(population);
    )";

    Program program;

    program.bind<std::string>("string");

    program.bind<Bear>("Bear")
        .bind("name", &Bear::name)
        .bind("weight", &Bear::weight)
        .bind("attack", &Bear::attack)
        .bind("defence", &Bear::defence);

    program.bind<Population>("Population").bind("bear", &Population::bear);

    program.bind("print_population", print_population);

    Compiler compiler;
    compiler.compile(program, source);
    program.run();

    print_population(program["population"].as<Population>());

    return 0;
}