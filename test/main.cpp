#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
        struct Time{
            float hour;
            float minute;
            float second;
        };

        Time time;

        time.hour = 10;
        print(time.hour);

        for(int i = 0;i < 10;i++){
            if(i == 5){
                print(100);
            }else if(i == 6){
                print(200);
            }else{
                print(i);
            }
        }
    )";

    Compiler compiler;
    compiler.compile(source);
    compiler.execute();

    return 0;
}