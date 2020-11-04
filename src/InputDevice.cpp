#include "InputDevice.hpp"
#include <unistd.h>
#include <errno.h>
#include <stdexcept>
InputDevice::InputDevice(int fd, bool deleteOnClose): fd(fd), deleteOnClose(deleteOnClose){}
InputDevice::InputDevice(const InputDevice & other): fd(other.fd), deleteOnClose(false){}
InputDevice::InputDevice(InputDevice &&other): fd(other.fd), deleteOnClose(other.deleteOnClose){
    other.fd = -1;
    other.deleteOnClose = false;
}


InputDevice::~InputDevice(){}

std::vector<input_event> InputDevice::readInputEvents(){
    std::vector<input_event> events;
    input_event ev;
    if(fd != -1){
        int bytesRead = read(fd, &ev, sizeof(input_event));
        while(bytesRead) {
            if(bytesRead == -1){
                if(errno == EBADF){
                    throw new std::runtime_error("Bad file descriptor in device");
                }else if(errno = EAGAIN){
                    break;
                }else{
                    throw new std::runtime_error(std::string("Error in read in device. errno: ")+std::to_string(errno));
                }
            }
            events.push_back(ev);
            bytesRead = read(fd, &ev, sizeof(input_event));
        }
    }
    return std::move(events);
}