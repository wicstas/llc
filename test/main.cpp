#include <llc/compiler.h>

using namespace llc;

int main() {
    Program program;

    program.source = R"(
        vectori record;

        record.push_back(17);
        record.push_back(32);
        record.push_back(60);

        printi(record[0]);
        printi(record[1]);
        printi(record[2]);
    )";

    program.bind("printi", print<int>);
    program.bind("prints", print<std::string>);

    program.bind<std::vector<int>>("vectori")
        .bind("size", &std::vector<int>::size)
        .bind("resize", (void(std::vector<int>::*)(size_t)) & std::vector<int>::resize)
        .bind("push_back", (void(std::vector<int>::*)(const int&)) & std::vector<int>::push_back);

    Compiler compiler;
    compiler.compile(program);
    program.run();

    return 0;
}