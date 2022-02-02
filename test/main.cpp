#include <llc/compiler.h>

using namespace llc;

int main() {
    Program program;

    program.source = R"(
        vectori create_vector(int size){
            vectori list;
            for(int i = 0; i < size; i++){
                if(list.size() < 2)
                    list.push_back(1);
                else
                    list.push_back(list[i - 2] + list[i - 1]);
            }
            return list;
        }

        vectori list = create_vector(10);

        for(int i = 0; i < list.size(); i++)
            printi(list[i]);
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

    auto& list = program["list"].as<vectori&>();

    print("size of list: ", list.size());
    for (int i = 0; i < (int)list.size(); i++)
        print("#", i, ": ", list[i]);

    return 0;
}