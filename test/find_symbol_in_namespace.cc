#include "sample_header.hh"

namespace S {
    int f;
}

int foo()
{
    S::f = 1;
}

namespace S {
    int bar() {
        f = 1;
        int j;
    }


    class R final {
        int i;
    };
}


