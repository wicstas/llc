#include <llc/compiler.h>

using namespace llc;

int main() {
    std::string source = R"(
        struct Time{
            float minute;
            float second;
        };

        Time time;
        time.minute = 0.0f;
        time.second = 0.0f;

        float second_to_minute_ratio = 1.0f/60.0f;
        float stride = 10.0f;

        for(int i = 0;i < 10;i++){

            if(time.second < 60.0f){
                print(time.second);
            }else{
                print(time.minute);
            }

            time.second = time.second + stride;
            time.minute = time.minute + stride * second_to_minute_ratio;

        }


    )";

    Compiler compiler;
    compiler.compile(source);
    compiler.execute();

    return 0;
}