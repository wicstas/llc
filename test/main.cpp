#include <llc/compiler.h>

using namespace llc;

void function_test() {
    try {
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
    )";

        // bind function
        program.bind("printi", print<int>);

        using vectori = std::vector<int>;

        // bind class and its methods
        program.bind<vectori>("vectori")
            .bind("resize", overload_cast<size_t>(&vectori::resize))
            .bind("push_back", overload_cast<const int&>(&vectori::push_back));

        Compiler compiler;
        compiler.compile(program);
        program.run();

        // get reference to varaible defined inside program
        auto& list = program["list"].as<vectori&>();

        // run function defined inside program
        for (int i = 5; i < 10; i++)
            list.push_back(program["fibonacci"](i).as<int>());

        for (int i = 0; i < (int)list.size(); i++)
            print("#", i, ": ", list[i]);

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}

void struct_test(){
        try {
        Program program;

        program.source = R"(
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

        Compiler compiler;
        compiler.compile(program);
        program.run();

        //call member function of struct defined inside program
        print("x.set(32);");
        program["x"]["set"](32);

        print("x.add(x.get());");
        program["x"]["add"](program["x"]["get"]().as<int>());

        print("x.get(): ", program["x"]["get"]().as<int>());

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}

int main() {
    function_test();
    struct_test();

    return 0;
}