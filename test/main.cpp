#include <llc/compiler.h>

using namespace llc;

int main() {
    Program program;

    program.source = R"(
        int fibonacci_impl(int a, int b, int n){
            if(n == 0)
                return a;
            else
                 return fibonacci_impl(b, a + b, n - 1);
        }
        
        int fibonacci(int n){
            return fibonacci_impl(0,1,n);
        }

        vectori list;

        for(int i = 0;i < 5;i++)
            list.push_back(fibonacci(i));

        struct Number{
            void set(int n){
                number = n;
            }
            int get(){
                return number;
            }

            void add(float n){
                number = number + n;
            }

            int number;
        };

        Number x;
        x.number = 10;
    )";

    // bind a function
    program.bind("printi", print<int>);
    program.bind("printf", print<float>);

    using vectori = std::vector<int>;

    // bind a class and its methods
    program.bind<vectori>("vectori")
        .bind("resize", overload_cast<size_t>(&vectori::resize))
        .bind("push_back", overload_cast<const int&>(&vectori::push_back));

    Compiler compiler;
    compiler.compile(program);
    program.run();

    // get variable from program
    try {
        // get a reference to a varaible defined inside program
        auto& list = program["list"].as<vectori&>();

        // run function defined inside program
        for (int i = 5; i < 10; i++)
            list.push_back(program["fibonacci"](i).as<int>());

        for (int i = 0; i < (int)list.size(); i++)
            print("#", i, ": ", list[i]);

        print("x.set(32);");
        program["x"]["set"](32);

        print("x.add(x.get());");
        program["x"]["add"](program["x"]["get"]().as<int>());

        print("x.get(): ", program["x"]["get"]().as<int>());

    } catch (const std::exception& exception) {
        print(exception.what());
    }

    return 0;
}