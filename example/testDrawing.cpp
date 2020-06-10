#include "frameBufferDrawer.h"
#include <iostream>
#include <signal.h>
using namespace std;

ScreenWriter s;

void SigIntHandler(int param){
    s.finalize();
    exit(1);
}

void coreDumpHandler(int param){
    s.finalize();
    abort();
}



int main(){


    struct sigaction action;
    action.sa_handler = SigIntHandler;
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    action.sa_handler = coreDumpHandler;
    sigaction(SIGSEGV, &action, NULL);

    try
    {


        s.show();
        s.background(255,0,255);
        s.color(0,255,0);
        s.rect(300,300,300,300);
        s.draw();
        cin.get();
    }
    catch(const char *smg)
    {
        std::cerr << smg << '\n';
    }
}
