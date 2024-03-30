#include "sample_header.hh"
void foo(int x, char y);

struct S {
    void foobar();
    void another_foo();
};

void baz()
{
    foo(123, 'b');
    bar();
    S s;
    s.foobar();
}

void S::foobar() {
    another_foo();
}