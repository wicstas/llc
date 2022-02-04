#include <llc/compiler.h>

using namespace llc;

void minimal_test() {
    Program program;
    program.source = R"(
        prints("Hello World!");
    )";
    program.bind(
        "prints", +[](std::string s) { std::cout << s << std::endl; });

    Compiler compiler;
    compiler.compile(program);
    program.run();
}

void function_test() {
    try {
        Program program;

        program.source = R"(
        int fibonacci_impl(int a, int b, int n){
            if(n <= 0)
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

void struct_test() {
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

void ctor_test() {
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
            .ctor<std::string>()
            .ctor<float>()
            .ctor<float, float, float>()
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

void dynamic_alloc_test() {
    try {
        Program program;

        struct vector {
            vector() = default;
            vector(int n, int* ptr) : n(n), ptr(ptr){};

            int& operator[](int i) {
                if (i >= n)
                    throw_exception("index out of range(range: [0, ", n, "), index: ", i, ")");
                return ptr[i];
            }
            const int& operator[](int i) const {
                if (i >= n)
                    throw_exception("index out of range(range: [0, ", n, "), index: ", i, ")");
                return ptr[i];
            }

            int n;
            std::shared_ptr<int[]> ptr;
        };

        program.source = R"(
        vector v = vector(1, new int);
        v[0] = 10;
        printsi("v[0] = ",v[0]);
    )";

        program.bind("printsi", print<std::string, int>);
        program.bind<vector>("vector").ctor<int, int*>();

        Compiler compiler;
        compiler.compile(program);
        program.run();

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}

void test() {
    try {
        Program program;

        program.source = R"(
        int i = 5;
        i += 5;
        )";

        Compiler compiler;
        compiler.compile(program);
        program.run();

        print("5 + 5 = ",program["i"].as<int>());

    } catch (const std::exception& exception) {
        print(exception.what());
    }
}

int main() {
    minimal_test();
    function_test();
    struct_test();
    ctor_test();
    dynamic_alloc_test();
    test();

    return 0;
}