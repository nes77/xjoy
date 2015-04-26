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

#define BOOST_ALL_DYN_LINK

#include "xjoy.h"
#include "xjoy_utils.hpp"



#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/unordered_map.hpp>

#include <string>

#include <algorithm>
#include <memory>
#include <queue>
#include <cstring>
#include <cstdlib>
#include <cstdio>


using namespace std;

const char* lib_id =
    "libxjoy ver " XJOY_VERSION " C 2015 Nicholas Samson under Apache License 2.0";

static boost::mutex xjoy_kill_all_access;
static boost::mutex xjoy_rtu_queue_access;
static boost::mutex xjoy_utd_queue_access;
static boost::mutex xjoy_callback_map_access;

static boost::condition_variable xjoy_update_available;
static boost::condition_variable xjoy_packet_available;

static std::unique_ptr<boost::thread_group> xjoy_threads_ptr;

static boost::unordered_map<uint32_t, xjoy_callback_func> xjoy_callbacks;
static queue<xjoy_controller_state_update> xjoy_reader_to_updater;
static queue<xjoy_packed_update_t> xjoy_updater_to_dispatch;


static bool xjoy_killed = false;

static bool xjoy_should_exit(){
    boost::lock_guard<boost::mutex> lg(xjoy_kill_all_access);
    return xjoy_killed;
}

extern "C" {

    void xjoy_init(const char* controller_device) {
        boost::lock_guard<boost::mutex> lg(xjoy_kill_all_access);
        xjoy_killed = false;
        
        xjoy_callbacks = boost::unordered_map<uint32_t, xjoy_callback_func>();
        xjoy_reader_to_updater = queue<xjoy_controller_state_update>();
        xjoy_updater_to_dispatch = queue<xjoy_packed_update_t>();
        
        xjoy_threads_ptr.reset(new boost::thread_group);
        xjoy_threads_ptr->create_thread(boost::bind(&xjoy::read_xjoy_stream,
                string(controller_device)));
        xjoy_threads_ptr->create_thread(boost::bind(&xjoy::update_xjoy_global_state));
        xjoy_threads_ptr->create_thread(boost::bind(&xjoy::dispatch_to_callbacks));
    }

    void xjoy_kill() {
        {
            boost::lock_guard<boost::mutex> lg(xjoy_kill_all_access);
            xjoy_killed = true;
        }
        xjoy_update_available.notify_all();
        xjoy_packet_available.notify_all();
        xjoy_threads_ptr->join_all();
    }
    
    void xjoy_set_button_callback(
            xjoy_callback_func callback_func,
            BUTTON_NAME which,
            BUTTON_STATE state){
        
        uint32_t id = 0x00000000U;
        
        if(which == CONTROLLER_OFF){
            id = CONTROLLER_OFF;
        }
        else {
            id =  ((uint32_t)which) << 16 | state;
        }
        
        boost::lock_guard<boost::mutex> lg(xjoy_callback_map_access);
        xjoy_callbacks[id] = callback_func;
        
    }
    
    void xjoy_unset_button_callback(
            BUTTON_NAME which,
            BUTTON_STATE state){
        uint32_t id = 0x00000000U;
        
        if(which == CONTROLLER_OFF){
            id = CONTROLLER_OFF;
        }
        else {
            id =  ((uint32_t)which) << 16 | state;
        }
        boost::lock_guard<boost::mutex> lg(xjoy_callback_map_access);
        xjoy_callbacks[id] = NULL;
    }
    
    void xjoy_set_ranged_input_callback(
            xjoy_callback_func callback_func,
            BUTTON_NAME which){
        uint32_t id = 0x00000000U;
        
        if(which == CONTROLLER_OFF){
            id = CONTROLLER_OFF;
        }
        else {
            id =  ((uint32_t)which) << 16;
        }
        boost::lock_guard<boost::mutex> lg(xjoy_callback_map_access);
        xjoy_callbacks[id] = callback_func;
    }
    
    void xjoy_unset_ranged_input_callback(
            BUTTON_NAME which){
        uint32_t id = 0x00000000U;
        
        if(which == CONTROLLER_OFF){
            id = CONTROLLER_OFF;
        }
        else {
            id =  ((uint32_t)which) << 16;
        }
        boost::lock_guard<boost::mutex> lg(xjoy_callback_map_access);
        xjoy_callbacks[id] = NULL;
    }

    void xjoy_print_update(xjoy_controller_state_update update) {
        printf("Timestamp: 0x%08x\nButton changed: 0x%04hx\nNew value: 0x%04hx\n\n",
                update.time_stamp,
                update.changed_input,
                update.new_value);
    }
}

#ifndef WIN32

#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

int
input_timeout (int filedes, unsigned int seconds)
{
  fd_set set;
  struct timeval timeout;
  /* Initialize the file descriptor set. */
  FD_ZERO (&set);
  FD_SET (filedes, &set);

  /* Initialize the timeout data structure. */
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;

  /* select returns 0 if timeout, 1 if input available, -1 if error. */
  return TEMP_FAILURE_RETRY (select (FD_SETSIZE,
                                     &set, NULL, NULL,
                                     &timeout));
}

void xjoy::read_xjoy_stream(string filename) {

    f_bin dev(filename);
    
    int file_desc = fileno(dev.get());

    while (!xjoy_should_exit()) {
        char data_line[8];

        xjoy_controller_state_update new_update;

        bool read_suc = false;
        while (!read_suc){
            if (input_timeout(file_desc, 3) <= 0){
                puts("Timed out.");
                if (xjoy_should_exit()){
                    return;
                }
            } else {
                read(file_desc, data_line, 8);
                read_suc = true;
            }
        }

        //Timestamp is lower 4 bytes, value next two, button pressed next two

        memcpy(&new_update.time_stamp, data_line, 4);
        memcpy(&new_update.new_value, data_line + 4, 2);
        memcpy(&new_update.changed_input, data_line + 6, 2);

        xjoy_print_update(new_update);

        {
            boost::lock_guard<boost::mutex> lg(xjoy_rtu_queue_access);
            xjoy_reader_to_updater.push(new_update);
            xjoy_update_available.notify_all();
        }

    }
}

#endif

void xjoy::update_xjoy_global_state() {

    while (!xjoy_should_exit()) {
        
        xjoy_packed_update_t upd;
        
        {
            boost::unique_lock<boost::mutex> ulock(xjoy_rtu_queue_access);

            while (xjoy_reader_to_updater.empty() && !xjoy_should_exit()) {
                xjoy_update_available.wait(ulock);
            }
            
            if (xjoy_should_exit()){
                puts("Dispatcher exiting.");
                return;
            }
            
            upd.update = xjoy_reader_to_updater.front();
            xjoy_reader_to_updater.pop();
        }
        
        if (upd.update.changed_input & 0x0080){
            upd.state.controller_off = true;
        }
        else {
            upd.state.controller_off = false;
        }
        
        switch (upd.update.changed_input){
            case A_BUTTON:
                upd.state.a_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case B_BUTTON:
                upd.state.b_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case Y_BUTTON:
                upd.state.y_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case X_BUTTON:
                upd.state.x_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case LEFT_BUMPER:
                upd.state.left_bumper = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case RIGHT_BUMPER:
                upd.state.right_bumper = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case BACK_BUTTON:
                upd.state.back_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case START_BUTTON:
                upd.state.start_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case HOME_BUTTON:
                upd.state.home_button = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case LEFT_STICK_BUTTON:
                upd.state.left_stick = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case RIGHT_STICK_BUTTON:
                upd.state.right_stick = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case DPAD_LEFT:
                upd.state.dpad_left = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case DPAD_RIGHT:
                upd.state.dpad_right = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case DPAD_UP:
                upd.state.dpad_up = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case DPAD_DOWN:
                upd.state.dpad_down = upd.update.new_value ?
                    (bool) BUTTON_PRESSED :
                    (bool) BUTTON_RELEASED;
                break;
            case LSTICK_X_AXIS:
                upd.state.left_stick_x = upd.update.new_value;
                break;
            case LSTICK_Y_AXIS:
                upd.state.left_stick_y = upd.update.new_value;
                break;
            case LTRIGGER:
                upd.state.left_trigger = upd.update.new_value;
                break;
            case RSTICK_X_AXIS:
                upd.state.right_stick_x = upd.update.new_value;
                break;
            case RSTICK_Y_AXIS:
                upd.state.right_stick_y = upd.update.new_value;
                break;
            case RTRIGGER:
                upd.state.right_trigger = upd.update.new_value;
                break;
            default:
                perror("Controller off?");
                break;
            
        }
        
        {
            boost::lock_guard<boost::mutex> lg(xjoy_utd_queue_access);
            xjoy_updater_to_dispatch.push(upd);
            xjoy_packet_available.notify_all();
        }
        
    }


}

void xjoy::dispatch_to_callbacks(){
    
    while (!xjoy_should_exit()){
        
        xjoy_packed_update_t upd;
        
        {
            boost::unique_lock<boost::mutex> ulock(xjoy_utd_queue_access);

            while (xjoy_updater_to_dispatch.empty() && !xjoy_should_exit()) {
                xjoy_packet_available.wait(ulock);
            }
            
            if (xjoy_should_exit()){
                puts("Dispatcher exiting.");
                return;
            }
            
            upd = xjoy_updater_to_dispatch.front();
            xjoy_updater_to_dispatch.pop();
        }
        
        {
            boost::unique_lock<boost::mutex> ulock(xjoy_callback_map_access);
            
            uint32_t id;
            xjoy_callback_func callback;
            
            if (upd.state.controller_off) {
                id = ((uint32_t)CONTROLLER_OFF);
            }
            else if (upd.update.changed_input & 0x0001U){
                id = ((uint32_t)upd.update.changed_input) << 16 | upd.update.new_value;
            }
            else {
                id = ((uint32_t)upd.update.changed_input) << 16;
            }
            
            callback = xjoy_callbacks[id];
            
            if (callback){
                callback(upd);
            }
            
        }
        
    }
    
}

