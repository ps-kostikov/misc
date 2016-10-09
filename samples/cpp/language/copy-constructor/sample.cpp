#include <iostream>
#include <memory>

class A
{
public:
    A() {
        std::cout << "A()" << std::endl;
    }

    A(const A&) {
        std::cout << "A(const A&)" << std::endl;
    }

    A& operator=(const A&) {
        std::cout << "operator=(const A&)" << std::endl;
        return *this;
    }

    ~A() {
        std::cout << "~A()" << std::endl;
    }

};


class B
{
public:
    B() {
        std::cout << "B()" << std::endl;
    }

    B(const B&) {
        throw std::runtime_error("error in B(const B&)");
        std::cout << "B(const B&)" << std::endl;
    }

    B& operator=(const B&) {
        std::cout << "operator=(const B&)" << std::endl;
        return *this;
    }

    ~B() {
        std::cout << "~B()" << std::endl;
    }

};

class Foo {
public:
  Foo(A* a, B* b): a(a), b(b) {}

  ~Foo() {
    delete a;
    delete b;
  }

  Foo(const Foo& other) {
    // wrong
    // a = new A(*other.a);
    // b = new B(*other.b);


    // better
    // A* aCopyPtr = nullptr;
    // B* bCopyPtr = nullptr;

    // try {
    //     aCopyPtr = new A(*other.a);
    //     bCopyPtr = new B(*other.b);
    // } catch (...) {
    //     if (aCopyPtr != nullptr) {
    //         delete aCopyPtr;
    //     }
    //     if (bCopyPtr != nullptr) {
    //         delete bCopyPtr;
    //     }
    //     throw;
    // }

    // a = aCopyPtr;
    // b = bCopyPtr;


    // good
    std::unique_ptr<A> aCopyPtr(new A(*other.a));
    std::unique_ptr<B> bCopyPtr(new B(*other.b));

    a = aCopyPtr.release();
    b = bCopyPtr.release();
  }
  // Write the copy constructor for the class Foo. 
  // Ownership of a and b should be maintained by Foo.
  // Types A and B are copy-constructible.

private:
  A* a;
  B* b;
};

int main()
try {
    A* a = new A();
    B* b = new B();

    Foo f(a, b);
    Foo fCopy(f);
    return 0;
} catch (std::exception& ex) {
    std::cout << "exception caught: " << ex.what() << std::endl;
    return 1;
}
