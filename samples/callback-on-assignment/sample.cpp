#include <iostream>

void do_callback() {
    std::cout << "callback" << std::endl;
}

class A
{
public:
    int i;
};


class AProxy
{
public:
    AProxy(A& a): a(a) {}

    A& operator()() {return a;}

    ~AProxy() {do_callback();}
private:
    A& a;
};

class AHolder
{
public:
    AHolder(A a) : a(std::move(a)) {}

    AProxy data() {return AProxy(a);}

    const A& constData() {return a;}
private:
    A a;
};

int main()
{
    A a{3};
    AHolder holder(a);

    std::cout << holder.constData().i << std::endl;
    holder.data()().i = 4;
    std::cout << holder.constData().i << std::endl;

    return 0;
}
