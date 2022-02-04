# llc, a tiny c++ interpreter

## Minimal example:
```c++
#include <llc/compiler.h>

void minimal_test() {

    llc::Program program;
    program.source = R"(
        prints("Hello World!");
    )";

    program.bind("prints", +[](std::string s) { std::cout << s << std::endl; });

    llc::Compiler compiler;
    compiler.compile(program);
    program.run();
}

```

## Quick Start
1. configure `CMakeLists.txt`.
```
add_subdirectory(path_to_llc_repo)

target_include_directories(project_name path_to_llc_repo/include)

target_link_libraries(project_name llc)
```

2. include *llc* header, all *llc* classes/functions are inside namespace `llc`
```
#include <llc/compiler.h>
```

## Examples
#### 1. bind function/class
```c++
#include <llc/compiler.h>
using namespace llc;

int main() {
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

        // get reference to variable defined inside program
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
```

#### 2. declare a struct and use its member function
```c++
#include <llc/compiler.h>
using namespace llc;

int main() {
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
        x.set(10);
    )";

        Compiler compiler;
        compiler.compile(program);
        program.run();

        print("x = ", program["x"]["get"]().as<int>());

        // call member function of struct defined inside program
        // x = 32
        program["x"]["set"](32);

        // x = x + x;
        program["x"]["add"](program["x"]["get"]().as<int>());

        print("x = ", program["x"]["get"]().as<int>());

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}
```

#### 3. bind a class with constructor
```c++
#include <llc/compiler.h>
using namespace llc;

int main() {
    try {
        Program program;

        struct Vec3 {
            Vec3() = default;
            Vec3(std::string s) : x(std::stof(s)), y(std::stof(s)), z(std::stof(s)) {
            }
            Vec3(float v) : x(v), y(v), z(v) {
            }
            Vec3(float x, float y, float z) : x(x), y(y), z(z) {
            }

            float x, y, z;
        };

        program.source = R"(
        printv( Vec3(1,2,3) );
        printv( Vec3(4) );
        printv( Vec3("5") );
    )";

        program.bind(
            "printv", +[](Vec3 v) { print(v.x, ',', v.y, ',', v.z); });
        program.bind<Vec3>("Vec3")
            .bind<std::string>()
            .bind<float>()
            .bind<float, float, float>()
            .bind("x", &Vec3::x)
            .bind("y", &Vec3::y)
            .bind("z", &Vec3::z);

        Compiler compiler;
        compiler.compile(program);
        program.run();

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}
```