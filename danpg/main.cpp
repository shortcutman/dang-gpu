//
//  main.cpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#include <iostream>
#include <fstream>

#include "jpeg.hpp"

int main(int argc, const char * argv[]) {
    
    std::fstream f("/Users/daniel/Projects.nosync/danpg/danpg/danpg/image2.jpg");
    
    if (f.bad()) {
        std::cout << "unable to open" << std::endl;
    } else {
        std::cout << "opened" << std::endl;
        
        image::Jpeg jpeg(f);
    }
    
    return 0;
}
