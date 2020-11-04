#include "../src/Engine.hpp"
#include "functional"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <array>

//#define PDB
//#define SDB

struct vec2d{
    double x;
    double y;
    vec2d(const mousePosition &mp) : x(mp.x), y (mp.y){}
    vec2d(double x = 0, double y = 0) : x(x), y(y){}
    //vec2(const vec2& other) x(other.x), x(other.y){}
    vec2d operator+(const vec2d& v) const {
        return vec2d(x + v.x, y + v.y);
    }

    vec2d operator-(const vec2d& v) const {
        return vec2d(x - v.x, y - v.y);
    }

    vec2d operator*(const double m) const{
        return vec2d(x * m, y * m);
    }

    vec2d operator/(const double d) const{
        return vec2d(x / d, y / d);
    }

    void operator+=(const vec2d& v) {
        this->x += v.x;
        this->y += v.y;
    }

    void operator-=(const vec2d& v) {
        this->x -= v.x;
        this->y -= v.y;
    }

    void operator*=(const double m) {
        this->x *= m;
        this->y *= m;
    }

    void operator/=(const double m) {
        this->x /= m;
        this->y /= m;
    }

    double mag(){
        return sqrt(x*x + y*y);
    }
};




std::ostream& operator<<(std::ostream& s, const vec2d& v){
    s << "( " << v.x << ", " << v.y << ")";
    return s;
}

struct Pointer{
    vec2d position;
    vec2d speed;
    Pointer(int x = 0, int y = 0 ):position(x, y), speed(0,0){}
};
struct GravityCore{
    vec2d position;
    GravityCore(int x, int y): position(x,y){}
    void move(int x, int y){
        this -> position = vec2d(x, y);
    }
};

#define threadNum 8

const struct{
    int mouseAcc = 2;
    int repell = 17;
    double fric = 0.0075;
    double maxSpd = 15;
    const int pointQty = 200000;
    const double gcAcc = 0.5;
    int threadAmount = threadNum;
} gameConstants;

class myGame:public Engine{
    private:
        //variables
        bool mousing=false;
        bool gravitating = true;
        bool spdLimit = false;
        Pointer *pointers; //allocate in constructor
        int coreQty = 0;
        std::vector<GravityCore> gravityCores;
    public:
        //methods
        myGame(){
            pointers = new Pointer[gameConstants.pointQty];
            resetPointers();
            background(255);
        }

        ~myGame(){
            delete[] pointers;
        }

        void resetPointers(){
            loopPointers([&](Pointer &p){
                p.position = {rand() % getWidth(), rand() % getHeight()};
                p.speed = vec2d();
            });
        }

        void addCore(int x,int y){
            if(coreQty < gravityCores.size()){
                gravityCores[coreQty].move(x,y);
            }else{
                gravityCores.push_back(GravityCore(x,y));
            }
            coreQty++;
        }

        void loopPointers(std::function<void(Pointer&)> callback){
            for(int i = 0; i < gameConstants.pointQty ; i++){
                callback(pointers[i]);
            }
        }

         void loopGravityCores(std::function<void(GravityCore&)> callback){
            for(int i = 0; i < coreQty ; i++){
                callback(gravityCores[i]);
            }
        }

        void stopPointers(){
            loopPointers([](Pointer& p){
                p.speed = vec2d(0,0);
            });
        }

        void drawPointer(const Pointer& p){
            //color(0);
            rect(p.position.x - 1, p.position.y - 1, 3, 3);
        }

        void drawGravityCore(const GravityCore& g){
            rect(g.position.x - 10, g.position.y - 10, 20, 20);
        }
        void updatePointer(Pointer &p){
#ifdef PDB
            std::cout << "Posizione:_____________________________________" << std::endl;
            std::cout << "Posizione puntatore prima: " << p.position << std::endl;
            std::cout << "velocità puntatore: " << p.speed << std::endl;
#endif
            p.position += p.speed;
#ifdef PDB
            std::cout << "Posizione puntatore dopo: " << p.position << std::endl;
            std::cout << "_______________________________________________" << std::endl;
#endif
            if(gravitating){
                //attrazione mouse
                if(keyPressed(BTN_LEFT)){
#ifdef SDB
                    std::cout << "Velocità con aggiornamento mouse:_____________________________________" << std::endl;
                    std::cout << "Velocità puntatore prima: " << p.speed << std::endl;
#endif
                    vec2d m2p = vec2d(mouse()) - p.position ;
                    double mag = m2p.mag();
                    p.speed += vec2d(gameConstants.mouseAcc*(m2p.x/mag), gameConstants.mouseAcc*(m2p.y/mag));
#ifdef SDB
                    std::cout << "Velocità puntatore dopo: " << p.speed << std::endl;
                    std::cout << "_______________________________________________" << std::endl;
#endif
                }
                //attrazione gravity cores
                for(int i = 0; i < coreQty; i++){
                    vec2d m2gc = gravityCores[i].position - p.position;
                    double magc = m2gc.mag();
                    p.speed += vec2d(gameConstants.gcAcc*(m2gc.x/magc), gameConstants.gcAcc*(m2gc.y/magc));

                }
                p.speed *= ((double)1 - gameConstants.fric);
                /*
                if(p.speed.mag() > constants.maxSpd && spdLimit){
                    vec2d rate = p.speed / p.speed.mag();
                    p.speed = rate * constants.maxSpd;
                }*/
            }
        }
        static void updatePointers(Pointer* pointers, int from, int to, const std::vector<GravityCore> &gravityCores, int coresQuantity, bool applyGravity, bool mousePressed, vec2d mouse);

        void onKeyUp(int key){
            if(key == KEY_ESC){
                close();
            }else if(key == KEY_O){// o 
                gravitating=!gravitating;
            }else if(key == KEY_SPACE){// ' '
                stopPointers();
            }else if(key == KEY_L){ // l
                spdLimit= !spdLimit;
            }else if(key == KEY_KPMINUS || key == KEY_SLASH){ // -
                coreQty--;
            }else if((key == KEY_RIGHTBRACE || key == KEY_KPPLUS) &&coreQty<gravityCores.size()){ // +
                coreQty++;
            }else if(key == KEY_KPSLASH){// /
                coreQty=0;
            }else if(key == KEY_MAIL){ // *
                coreQty=gravityCores.size();
            }else if(key == KEY_A){
                addCore(mouse().x,mouse().y);
            }else if(key == KEY_DOT){
                resetPointers();
            }
        }


        virtual void draw(){
            background(255);
	        color(255,0,0);
            loopGravityCores([&](GravityCore&  g){
                drawGravityCore(g);
            });
	        color(0);
            /* multithreading test :(
            std::thread threads[threadNum];
            int startIndex = 0;

            for(int i = 0; i < gameConstants.threadAmount-1; i++){
                int endIndex = gameConstants.pointQty / gameConstants.threadAmount * i;
                threads[i] = std::thread(myGame::updatePointers, pointers, startIndex, endIndex, gravityCores, coreQty, gravitating, keyPressed(BTN_LEFT), mouse());
                startIndex = endIndex;
            }
            updatePointers(pointers, startIndex, gameConstants.pointQty, gravityCores, coreQty, gravitating, keyPressed(BTN_LEFT), mouse());
            for(int i = 0; i < gameConstants.threadAmount-1; i++){
                threads[i].join();
            }

            loopPointers([&](Pointer& p){   
                drawPointer(p);
            });*/
            loopPointers([&](Pointer& p){   
                updatePointer(p);
                drawPointer(p);
            });
            color(0,0,255);
            rect(mouse().x - 5, mouse().y - 5, 10, 10);
        }
}; 

void myGame::updatePointers(Pointer* pointers, int from, int to, const std::vector<GravityCore> &gravityCores, int coresQuantity, bool applyGravity, bool mousePressed, vec2d mouse){
    for(int i = from; i < to; i++){
        Pointer &p = pointers[i];
        p.position += p.speed;
        /*std::cout << "Posizione puntatore dopo: " << p.position << std::endl;
        std::cout << "_______________________________________________" << std::endl;*/
        if(applyGravity){
            //attrazione mouse
            if(mousePressed){
                vec2d m2p = mouse - p.position ;
                double mag = m2p.mag();
                p.speed += vec2d(gameConstants.mouseAcc*(m2p.x/mag), gameConstants.mouseAcc*(m2p.y/mag));
            }
            //attrazione gravity cores
            for(int i = 0; i < coresQuantity; i++){
                vec2d m2gc = gravityCores[i].position - p.position;
                double magc = m2gc.mag();
                p.speed += vec2d(gameConstants.gcAcc*(m2gc.x/magc), gameConstants.gcAcc*(m2gc.y/magc));

            }
            p.speed *= 1 - gameConstants.fric;
            /*
            if(p.speed.mag() > constants.maxSpd && spdLimit){
                vec2d rate = p.speed / p.speed.mag();
                p.speed = rate * constants.maxSpd;
            }*/
        }
    }
    std::cout << std::this_thread::get_id << " finished" << std::endl;
}

int main(){
    srand(time(NULL));
    myGame game;
    game.start();
    std::cin.ignore(2147483647);
}
