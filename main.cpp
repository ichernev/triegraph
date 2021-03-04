#include "str.h"
#include "trigraph.h"
#include <iostream>

int main() {
    // str<dna_letter> s;

    // std::cin >> s;
    // std::cout << s << '\n';
    // std::cout << "got " << s.length << " chars" << std::endl;
    // std::cout << "got " << s.capacity << " holders" << std::endl;
    // str<dna_letter> s;
    // std::cout << "options " << dna_letter::num_options << std::endl;
    // std::cout << "num bits " << dna_letter::bits << std::endl;

    auto tg = dfl_trigraph::from_rgfa("data/simple.gfa");

    return 0;
}
