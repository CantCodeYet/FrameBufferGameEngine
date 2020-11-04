#include "Engine.hpp"
#include <string>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "NoDeviceLoadedError.hpp"

#define KEY_PRESSED 1
#define KEY_RELEASED 0

//used for device loading
static std::string exec(const char* cmd) {
    char buffer[1024];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("Command execution failed.");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

//used for device loading
static void execOnSplit(const std::string &target, const std::string &delimiter, const std::function<void(const std::string&)> callback){

    auto start = 0U;
    auto end = target.find(delimiter);
    while (end != std::string::npos)
    {
        callback(target.substr(start, end - start));
        start = end + delimiter.length();
        end = target.find(delimiter, start);
    }

    if(target.substr(start, end).length() > 0)
        callback(target.substr(start, end));
}

Engine::Engine(): shouldClose(false), updateGraphics(true),mousePos({ScreenWriter::getWidth() / 2, ScreenWriter::getHeight() / 2})
{
    loadDevices();
};

Engine::~Engine(){
    
}

void Engine::addDevice(InputDevice&& device){
    devices.push_back(device);
}

void Engine::loadDevices(){
    std::string result = exec("ls -lRa /dev/input/by-path/ | grep -E \"(event-kbd|event-mouse)\" | awk '{value = $NF; num = split(value, arr, \"/\"); printf \"/dev/input/%s\\n\", arr[num]}'");
    std::vector<int> fds;
    auto callback = [&fds](std::string in){fds.push_back(open(in.c_str(), O_RDONLY | O_NONBLOCK));};
    execOnSplit(result, "\n", callback);
    for (auto fd : fds){
        if( fd != -1 ){
            InputDevice device(fd);
            addDevice(std::move(device));
        }
    }
    if(devices.size() == 0){
        throw new NoDeviceLoadedError();
    }
}

void Engine::start(){
    show();
    while(!shouldClose){
        updateInput();
        if(updateGraphics)
            draw();
        applyBuffer();
    }
    hide();
}

void Engine::updateInput(){
    int screenWidth = ScreenWriter::getWidth();
    int screenHeight = ScreenWriter::getHeight();
    int wheelRel = 0;
    for(auto d : devices){
        std::vector<input_event> events(d.readInputEvents());
        for(auto ev : events){
            switch (ev.type){
                case EV_REL:
                    if(ev.code == REL_X){
                        mousePos.x += ev.value;
                        if(mousePos.x < 0)
                            mousePos.x = 0;
                        else if(mousePos.x > screenWidth)
                            mousePos.x = screenWidth;
                    }else if(ev.code == REL_Y){
                        mousePos.y += ev.value;
                        if(mousePos.y < 0)
                            mousePos.y = 0;
                        else if(mousePos.y > screenHeight)
                            mousePos.y = screenHeight;
                    }else if(ev.code == REL_WHEEL){
                        wheelRel += ev.value;
                    }
                    break;
                case EV_KEY:
                    keyboard[ev.code] = ev.value;
                    if(ev.value == KEY_PRESSED){
                        onKeyDown(ev.code);
                    }else if(ev.value == KEY_RELEASED){
                        onKeyUp(ev.code);
                    }else if(ev.value == 2){
                        onKeyHeld(ev.code);
                    }else{
                        throw new std::runtime_error("unknown value on key event: " + std::to_string(ev.value)); //for dev purpose
                    }
                    break;
            }
        }
    }
    if(wheelRel)
        onMouseWheelScroll(wheelRel);
}

bool Engine::keyPressed(int key){
    return keyboard[key] > 0;
}

//input events

void Engine::onKeyDown(int pressedKey){}
void Engine::onKeyUp(int pressedKey){}
void Engine::onKeyHeld(int pressedKey){}

void Engine::onMouseWheelScroll(int amount){}

void Engine::pause(){
    updateGraphics = false;
} //processing noLoop();

void Engine::play(){
    updateGraphics = true;
} //

void Engine::close(){
    shouldClose = true;
}

const mousePosition& Engine::mouse(){
    return mousePos;
}