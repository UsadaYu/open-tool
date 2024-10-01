#include "common.h"

int main() {

    fun_c1();
    fun_c2();
    C_INFO();

    fun_cxx1();
    fun_cxx2();
    CXX_INFO();

    /**
     * auto 在c++98标准中不可以这样使用，
     * 修改 CMakeLists.txt 中的 `CMAKE_CXX_STANDARD` 变量可验证。
     * 此时执行make命令时，编译会报错
     */
    auto cxx_auto_int = 11;
    auto cxx_auto_string = "UsadaYu";
    CXX_TYPE_INFO(cxx_auto_int);        // integer(i/int)
    CXX_TYPE_INFO(cxx_auto_string);     // pointer const char(PKc/const char *)

    return 0;
}
