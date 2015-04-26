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
#include <stdio.h>
#include <stdlib.h>

XJOY_CALLBACK(a_button_pressed){
    puts("Callback to a worked!");
    xjoy_print_update(update.update);
}

int main(int argc, char** argv) {
    
    int a;
    
    xjoy_init("/dev/input/js0");
    xjoy_set_button_callback(a_button_pressed, A_BUTTON, BUTTON_PRESSED);
    
    scanf("%d", &a);
    
    xjoy_kill();
    
    return 0;
}