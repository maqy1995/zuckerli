#include "zuckerli.h"
#include <ostream>
#include <iostream>
#include <cassert>
int main(){
    auto coo_pair = ReadGraphAndSampleNeighbors(10,2);
    auto srcs = coo_pair.first;
    auto dsts = coo_pair.second;
    assert(srcs.size() == dsts.size());
    for(auto src: srcs){
	    std::cout << src<< " ";
    }
    for(auto dst: dsts){
        std::cout << dst << " ";
    }
    std::cout << std::endl;
}
