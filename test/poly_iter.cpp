#include <iostream>
#include <string.h>

struct A {
    int storage;
    virtual int get() {
        return 0;
    }
};

struct AX : A {
    double another;
    virtual int get() {
        return 1;
    }
};

struct AY : A {
    int x[20];
    virtual int get() {
        return 2;
    }
};

union Holder {
    A a;
    AX ax;
    AY ay;

    Holder() { }
    Holder(const Holder &h) {
        memcpy(this, &h, sizeof(Holder));
    }

    void setA() {
        new(&a) A();
    }
    void setAX() {
        new(&ax) AX();
    }
    void setAY() {
        new(&ay) AY();
    }
    // Holder(A a) {
    //     wow.a = a;
    //     std::cerr << "A" << std::endl;
    // }
    // Holder(AX ax) {
    //     wow.ax = ax;
    //     std::cerr << "AX" << std::endl;
    // }
    // Holder(AY ay) {
    //     wow.ay = ay;
    //     std::cerr << "AY" << std::endl;
    // }

    static Holder make_a() {
        Holder h;
        h.setA();
        return h;
    }

    static Holder make_ax() {
        Holder h;
        h.setAX();
        return h;
    }

    static Holder make_ay() {
        Holder h;
        h.setAY();
        return h;
    }

    int get() {
        return reinterpret_cast<A*>(this)->get();
    }
};

int main() {
    std::cerr << Holder::make_a().get() << std::endl;
    std::cerr << Holder::make_ax().get() << std::endl;
    std::cerr << Holder::make_ay().get() << std::endl;
    return 0;
}
