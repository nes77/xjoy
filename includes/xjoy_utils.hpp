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

#ifndef XJOY_UTILS_HPP
#define	XJOY_UTILS_HPP

#include <cstdio>

class f_bin {
private:
    FILE* file;

public:

    f_bin(const std::string& file_path) {
        file = fopen(file_path.c_str(), "rb");
    }

    ~f_bin() {
        fclose(file);
    }

    FILE* get() {
        return file;
    }
};



#endif	/* XJOY_UTILS_HPP */

