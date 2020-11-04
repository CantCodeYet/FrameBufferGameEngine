#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__
#include "FrameBufferGraphics.hpp"
#include "InputDevice.hpp"
#include <vector>
#include <linux/input-event-codes.h>

typedef struct{
    int x;
    int y;
}  mousePosition;

class Engine : protected ScreenWriter{
    private:
        bool shouldClose;
        bool updateGraphics;
        std::vector<InputDevice> devices;
        unsigned char keyboard[KEY_CNT] = {0};
        mousePosition mousePos;
    private:
        void updateInput();
    protected:
        void addDevice(InputDevice&& device);
        virtual void loadDevices();
        //input events
        virtual void onKeyDown(int key);
        virtual void onKeyUp(int key);
        virtual void onKeyHeld(int key);
        virtual void onMouseWheelScroll(int amount);
        //check key pressed
        bool keyPressed(int key);
        //drawing function
        virtual void draw() = 0;
        const mousePosition& mouse();
        void close(); //sets the closing flag
        void pause(); //processing noLoop();
        void play();  //processing loop();
    public:
        Engine();
        ~Engine();
        void start();
};


#endif