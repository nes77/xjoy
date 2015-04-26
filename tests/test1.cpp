/* 
   Copyright 2015 Nicholas Samson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "xjoy.h"
#include <cstdlib>
//#include <iostream>
//#include <fstream>
#include <cstdint>

using namespace std;

/*
 * 
 */
int maint(int argc, char** argv) {

    
//    ifstream myFile ("/dev/input/js0", ios::in | ios::binary);
//    
//    while (true){
//    char buf[8];
//
//        myFile.read(buf, 8);
//
//        int64_t* i_ptr = reinterpret_cast<int64_t*> (buf);
//
//        printf("0x%016llx\n", *i_ptr);
//    }
//    
//    cout << sizeof(xjoy_packed_update_t) << endl;
    
    xjoy::read_xjoy_stream("/dev/input/js0");
    
    return 0;
}

