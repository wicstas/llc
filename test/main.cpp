#include <llc/compiler.h>

using namespace llc;

struct Bear {
    std::string name;
    float weight;
    float attack;
    float defence;
};

void print_bear(Bear bear) {
    print("Bear: ", bear.name);
    print("  weight : ", bear.weight);
    print("  attack : ", bear.attack);
    print("  defence: ", bear.defence);
}

int main() {
    std::string source = R"(
        Bear bear;
        bear.name = "a bear";
        bear.weight = 200.0f;
        bear.attack = 12.0f;
        bear.defence = 8.0f;

        print_bear(bear);

        struct Bird{
            string name;
            float weight;
            float speed;
        };

        void print_bird(Bird bird){
            printss("Bird: ",bird.name);
            printsf("  weight: ",bird.weight);
            printsf("  speed : ",bird.speed);
        }

        Bird create_bird(float weight,float speed){
            Bird bird;
            // if(weight < 1.0f)
                bird.name = "a bird";
            // else if(weight < 4.0f)
            //     bird.name = "A Bird";
            // else
            //     bird.name = "THE BIRD";

            bird.weight = weight;
            bird.speed = speed;
            return bird;
        }


        print_bird(create_bird(0.4f,10.0f));
        print_bird(create_bird(6.0f,10.0f));
        print_bird(create_bird(27.0f,10.0f));
    )";

    Program program;

    program.bind<std::string>("string");

    program.bind<Bear>("Bear")
        .bind("name", &Bear::name)
        .bind("weight", &Bear::weight)
        .bind("attack", &Bear::attack)
        .bind("defence", &Bear::defence);

    program.bind("printss", print<std::string, std::string>);
    program.bind("printsf", print<std::string, float>);
    program.bind("print_bear", print_bear);

    Compiler compiler;
    compiler.compile(program, source);
    program.run();

    return 0;
}