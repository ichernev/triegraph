#include <iostream>
#include <string.h>

struct A {
    int storage;
    virtual int get() {
        return 0;
    }
    void inc() {
        ++storage;
    }
    int state() const {
        return storage;
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
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wclass-memaccess"
    Holder(const Holder &h) { memcpy(this, &h, sizeof(Holder)); }
    #pragma GCC diagnostic pop

    template <typename T, T Holder::*field>
    void setX() {
        new(&(this->*field)) T();
    }

    void setA() {
        setX<A, &Holder::a>();
        // new(&a) A();
    }
    void setAX() {
        setX<AX, &Holder::ax>();
        // new(&ax) AX();
    }
    void setAY() {
        setX<AY, &Holder::ay>();
        // new(&ay) AY();
    }

    Holder end_holder() {
        Holder end;
        end.setA();
        end.a.storage = get() + 2;
        return end;
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

    void inc() {
        reinterpret_cast<A*>(this)->inc();
    }

    int state() const {
        return reinterpret_cast<const A*>(this)->state();
    }
};

struct iter {
    using value_type = int;
    using reference_type = int;
    using Self = iter;

    Holder h;

    reference_type operator*() {
        return h.get();
    }

    Self &operator++() { h.inc(); return *this; }
    Self operator++ (int) { Self tmp; ++(*this); return tmp; }

    bool operator== (Self &other) const { return h.state() == other.h.state(); }
};

struct iter_helper {

    iter_helper(Holder &&h) : it(std::move(h)) {}

    iter begin() { return it;  }
    iter end() { return iter(it.h.end_holder()); }

    iter it;
};

int main() {
    // std::cerr << Holder::make_a().get() << std::endl;
    // std::cerr << Holder::make_ax().get() << std::endl;
    // std::cerr << Holder::make_ay().get() << std::endl;
    for (auto i : iter_helper(Holder::make_ax())) {
        std::cerr << "ax " << i << std::endl;
    }
    for (auto i : iter_helper(Holder::make_ay())) {
        std::cerr << "ay " << i << std::endl;
    }
    return 0;
}
