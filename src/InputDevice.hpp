#ifndef __INPUTDEVICE__HPP__
#define __INPUTDEVICE__HPP__
#include <linux/input.h>
#include <vector>
#include <string>

class InputDevice{
private:
    int fd;
    bool deleteOnClose;
public:
    InputDevice(int, bool = true);
    InputDevice(const InputDevice &); //copy constructor
    InputDevice(InputDevice &&); //move constructor
    ~InputDevice();
    std::vector<input_event> readInputEvents();
};

#endif