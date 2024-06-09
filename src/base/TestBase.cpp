/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 17:41:00
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 15:40:04
 * @FilePath: /tmms/src/base/TestBase.cpp
 * @Description:  learn 
 */

#include "Singleton.h"
#include "StringUtils.h"
#include <iostream>
#include <string>
#include <vector>
using namespace tmms::base;

// 使用单例类就要保证不可复制，不可移动
class A : public NonCopyable
{
public:
    A() = default;
    ~A() = default;

public:
    void Print()
    {
        std::cout << "This is A!!!" << std::endl;
    }
};

void TestStringStateMachine()
{
    std::string str = "";
    std::string str1 = "a";
    std::string str2 = "aa;ab;ac;ad;ae";
    std::string str3 = ";;;;;";
    const char de = ';';

    std::vector<std::string> list = tmms::base::StringUtils::SplitStringWithFSM(str1, de);
    std::cout << "delimiter : " << de << " str1: " << str1 << " result size: " << list.size() << std::endl;
    for(auto v : list)
    {
        std::cout << v << std::endl;
    }

    list = tmms::base::StringUtils::SplitStringWithFSM(str2, de);
    std::cout << "delimiter : " << de << " str2: " << str2 << " result size: " << list.size() << std::endl;
    for(auto v : list)
    {
        std::cout << v << std::endl;
    }

    list = tmms::base::StringUtils::SplitStringWithFSM(str3, de);
    std::cout << "delimiter : " << de << " str3: " << str3 << " result size: " << list.size() << std::endl;
    for(auto v : list)
    {
        std::cout << v << std::endl;
    }
}

// 全局变量只能在函数外初始化，不能赋值
// 这里后面是个函数，会先初始化为空，然后赋值给sA，就报错了
// auto sA = tmms::base::Singleton<A>::Instance();

// 如 int a; a = 0; 在全局都是错的

#define sA tmms::base::Singleton<A>::Instance()
int main(int argc, char* argv[])
{
    // sA->Print();
    TestStringStateMachine();
    return 0;
}