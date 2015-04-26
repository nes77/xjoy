/*
 * xjoy - Gamepad controller library
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

#ifndef XJOY_H
#define	XJOY_H

#define A_BUTTON 0x0001
#define B_BUTTON 0x0101
#define X_BUTTON 0x0201
#define Y_BUTTON 0x0301

#define LEFT_BUMPER 0x0401
#define RIGHT_BUMPER 0x0501

#define BACK_BUTTON 0x0601
#define START_BUTTON 0x0701
#define HOME_BUTTON 0x0801

#define LEFT_STICK_BUTTON 0x0901
#define RIGHT_STICK_BUTTON 0x0A01

#define DPAD_LEFT 0x0B01
#define DPAD_RIGHT 0x0C01
#define DPAD_UP 0x0D01
#define DPAD_DOWN 0x0E01

#define BUTTON_PRESSED 0x0001
#define BUTTON_RELEASED 0x0000

#define LSTICK_X_AXIS 0x0002
#define LSTICK_Y_AXIS 0x0102
#define LTRIGGER 0x0202

#define RSTICK_X_AXIS 0x0302
#define RSTICK_Y_AXIS 0x0402
#define RTRIGGER 0x0502

#define XJOY_CALLBACK(callback_name) void callback_name(xjoy_packed_update_t update)
#define BUTTON_NAME uint16_t
#define BUTTON_STATE uint16_t

#define RANGED_INPUT_NAME uint16_t

#define CONTROLLER_OFF 0xFFFF

#define XJOY_BUFFER_SIZE 2048
#define XJOY_READ_TIMEOUT 3

#include <stdint.h>
#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

    
    
    typedef struct {
        uint32_t time_stamp;
        uint16_t new_value;
        uint16_t changed_input;
    } xjoy_controller_state_update;
    
    typedef struct {
        bool controller_off;
        
        bool a_button;
        bool b_button;
        bool x_button;
        bool y_button;
        
        bool left_bumper;
        bool right_bumper;
        bool back_button;
        
        bool start_button;
        bool home_button;
        
        bool left_stick;
        bool right_stick;
        
        bool dpad_left;
        bool dpad_right;
        bool dpad_up;
        bool dpad_down;
        
        uint16_t left_stick_x;
        uint16_t left_stick_y;
        
        uint16_t right_stick_x;
        uint16_t right_stick_y;
        
        uint16_t left_trigger;
        uint16_t right_trigger;
        
    } xjoy_controller_state_t;
    
    typedef struct {
        xjoy_controller_state_update update;
        xjoy_controller_state_t state;
    } xjoy_packed_update_t;
    
    typedef void (*xjoy_callback_func)(xjoy_packed_update_t);
    
    void xjoy_init(const char* controller_device);
    
    void xjoy_set_button_callback(
            xjoy_callback_func callback_func,
            BUTTON_NAME which,
            BUTTON_STATE state);
    
    void xjoy_unset_button_callback(
            BUTTON_NAME which,
            BUTTON_STATE state);
    
    void xjoy_set_ranged_input_callback(
            xjoy_callback_func callback_func,
            BUTTON_NAME which);
    
    void xjoy_unset_ranged_input_callback(
            BUTTON_NAME which);
    
    void xjoy_print_update(xjoy_controller_state_update update);
    
    void xjoy_kill();

#ifdef	__cplusplus
}
#endif

#ifdef __cplusplus

#include <string>
namespace xjoy {
    void read_xjoy_stream(std::string filename);
    void update_xjoy_global_state();
    void dispatch_to_callbacks();
}

#endif

#endif	/* XJOY_H */

