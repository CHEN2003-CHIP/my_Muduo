#pragma once 

#include <string>
#include <iostream>
using namespace std;

class Timestamp{
public:
    Timestamp();
    Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};