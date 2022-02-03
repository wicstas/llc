#include <llc/compiler.h>

using namespace llc;

int main() {
    Program program;

    program.source = R"(
        struct vector{
            void create(int size){
                for(int i = 0; i < size; i++){
                    if(base.size() < 2)
                        base.push_back(1);
                    else
                        base.push_back(base[i - 2] + base[i - 1]);
                }
            }

            void print(){
                for(int i = 0; i < base.size(); i++)
                    printi(base[i]);
            }

            vectori base;
            int number = 0;
        };

        vector vec;
        vec.create(10);
        vec.print();

        vectori list = vec.base;
    )";

    program.bind("printi", print<int>);

    using vectori = std::vector<int>;

    program.bind<vectori>("vectori")
        .bind("size", &vectori::size)
        .bind("resize", overload_cast<size_t>(&vectori::resize))
        .bind("push_back", overload_cast<const int&>(&vectori::push_back));

    Compiler compiler;
    compiler.compile(program);
    program.run();

    try {
        auto& list = program["list"].as<vectori&>();

        print("size of list: ", list.size());
        for (int i = 0; i < (int)list.size(); i++)
            print("#", i, ": ", list[i]);

    } catch (const std::exception& exception) {
        print(exception.what());
    }

    return 0;
}