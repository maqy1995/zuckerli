#include <gflags/gflags.h>
#include <iostream>
//
// Created by maqy on 23-6-6.
//
DEFINE_bool(my_flag, false, "A boolean flag");

int main(int argc, char **argv) {
    // 初始化gflags库
    google::ParseCommandLineFlags(&argc, &argv, true);

    // 访问传递的bool类型标志
    if (FLAGS_my_flag) {
        std::cout << "my_flag is true" << std::endl;
    } else {
        std::cout << "my_flag is false" << std::endl;
    }

    // 其他代码...

    return 0;
}