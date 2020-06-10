#include <cstdint>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstring>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <finalizable.h>

#define fbdev "/dev/fb0"
#define ttydev "/dev/tty"
#define MOD_NON 0 //no set mode
#define MOD_TXT 1 //text mode
#define MOD_GRA 2 //graphic mode
#define checkInit() \
if(!initialized){\
	throw "ScreenWriter not initialized or already finalized. Initialize again to use.";\
}

using namespace std;

typedef struct{
	uint8_t b, g, r, a;
} pixel;

class ScreenWriter : public Finalizable{
private:
	bool initialized;
	pixel usedColor;
	//uint16_t marginX;
	//uint16_t marginY;
	uint8_t ttyMode;
	bool colorNegative;
	struct{
		uint32_t framebuffer;
		uint32_t tty;
	} fd;
	pixel *buffer;
	pixel *nextBuffer;
	size_t size;
	uint16_t width;
	uint16_t height;

public:
	ScreenWriter();
	
	~ScreenWriter();
	
	uint16_t getWidth();
	
	uint16_t getHeight();
	/*
	sets margin to the screen
	the margin are symmetrical
	the usable width decreases by twice the value of x
	the usable height decreases by twice the value of y	
	*/
	//void setMargin(uint16_t x, uint16_t y);
	
	//int getMarginX();
	
	//int getMarginY();
	
	//void setMarginX(uint16_t mx);
		
	//void setMarginY(uint16_t my);
	
	void background(uint8_t r, uint8_t g, uint8_t b); //draws the background using the defined color
	
	void background(uint8_t greyScale); //draws the background using the defined color
		
	//void showScreen();//set console to graphic mode and hide screen
	
	void show();//stops the console from displaying and enables to draw on the screen
		
	void hide();//set console to text mode and stop drawing
	
	void init();//prepare the screen writer for writing
	
	void finalize();//close the screenWriter
	
	void draw();//draws the prepared buffer to the actual buffer

	void negative();//negative effect
	
	void negative(bool);//negative effect
	
	void rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height); //draws a rectangle
	
	//void drawPixel(uint16_t x, uint16_t y,uint16_t dimIncrease); //draws a pixel on the screen

	void drawPixel(uint16_t x, uint16_t y); //draws a pixel on the screen
	
	void color(uint8_t r, uint8_t g, uint8_t b); //sets the color used to draw
	
	void color(uint8_t greyScale); //sets the color used to draw
	
	//void line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t dimIncrease); //draws a line from (x0,y0) to (x1,y1)

	void line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1); //draws a line from (x0,y0) to (x1,y1)
};

//METHOD IMPLEMENTATION

ScreenWriter::ScreenWriter(){
	initialized = false;
	colorNegative = false;
	fd.framebuffer = -1;
	fd.tty = -1;
	buffer = NULL;
	nextBuffer = NULL;
	//marginX = 0;
	//marginY = 0;
	usedColor = {0};
	init();
}

ScreenWriter::~ScreenWriter(){
	finalize();
}

void ScreenWriter::init(){
	if(initialized)
		return;
	//get terminal configuration
	fd.tty = open(ttydev, O_RDWR);
	if(fd.tty < 0){
		throw "Cannot access tty device";
	}

	int mod;
	if(ioctl(fd.tty, KDGETMODE, &mod) == -1){
		ttyMode = MOD_NON;
	}else{
		if(mod == KD_TEXT){
			ttyMode = MOD_TXT;
		}else if(mod == KD_GRAPHICS){
			ttyMode = MOD_GRA;
		}else{
			ttyMode = MOD_NON;
		}
	}
	
	//get framebuffer configuration
	fd.framebuffer = open(fbdev, O_RDWR);
	if(fd.framebuffer < 0){
		finalize();
		throw "Cannot access framebuffer device";
	}
	
	fb_fix_screeninfo fix;
	fb_var_screeninfo var;
	
	
	if(ioctl(fd.framebuffer, FBIOGET_FSCREENINFO, &fix) == -1){
		finalize();
		throw "Cannot access framebuffer fixed info";
	} 
	
	
	if(ioctl(fd.framebuffer, FBIOGET_VSCREENINFO, &var) == -1){
		finalize();
		throw "Cannot access framebuffer variable info";
	}
	
	width = var.xres;
	height = var.yres;
	size = width * height * sizeof(pixel);
	
	//map framebuffer to buffer
	buffer = (pixel*)mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.framebuffer, 0);
	
	
	if(buffer == MAP_FAILED){
		finalize();
		throw "Cannot map framebuffer to memory";
	}
	
	nextBuffer = (pixel*)malloc(size);
	if(nextBuffer == NULL){
		finalize();
		throw "Cannot create prepared buffer";
	}
	
	memset(nextBuffer, 0, size);
	
	initialized = true;
}

void ScreenWriter::show(){
	checkInit();
	if(ttyMode != MOD_GRA){	
		if(ioctl(fd.tty, KDSETMODE, KD_GRAPHICS) == -1){
			throw "Cannot set graphic mode";
		}
		ttyMode = MOD_GRA;
	}
}

void ScreenWriter::hide(){
	checkInit();
	if(ttyMode != MOD_TXT){	
		if(ioctl(fd.tty, KDSETMODE, KD_TEXT) == -1){
			throw "Cannot set graphic mode";
		}
		ttyMode = MOD_TXT;
	}
}

void ScreenWriter::finalize(){
	if(fd.tty != -1){
		hide();
		if(close(fd.tty) == -1){
			throw "Unable to close tty file descriptor";
		}
		fd.tty = -1;
	}
	
	if(buffer != NULL && buffer != MAP_FAILED){
		if(munmap(buffer, size) == -1){
			throw "Cannot unmap framebuffer from memory";
		}
		buffer = NULL;
	}
	
	if(fd.framebuffer != -1){
		if(close(fd.framebuffer) == -1){
			throw "Unable to close framebuffer file descriptor";
		}
		fd.framebuffer = -1;
	}
	
	free(nextBuffer);
	nextBuffer = NULL;
	
	initialized = false;
}

uint16_t ScreenWriter::getWidth(){
	checkInit();
	return width;
}

uint16_t ScreenWriter::getHeight(){
	checkInit();
	return height;
}
/*
int ScreenWriter::getMarginX(){
	return marginX;
}

int ScreenWriter::getMarginY(){
	return marginY;
}

void ScreenWriter::setMarginX(uint16_t mx){
	if(mx > width/2){
		mx = width/2;
	}
	marginX = mx;
}

void ScreenWriter::setMarginY(uint16_t my){
	if(my > height/2){
		my = height/2;
	}
	marginY = my;
}

void ScreenWriter::setMargin(uint16_t x, uint16_t y){
	setMarginX(x);
	setMarginY(y);
}*/

void ScreenWriter::draw(){
	checkInit();
	if(!ttyMode == MOD_GRA){
		show();
	}
	memcpy(buffer, nextBuffer, size);
}

void ScreenWriter::negative(){
	negative(!colorNegative);
}

void ScreenWriter::negative(bool neg){
	if(colorNegative != neg){
		colorNegative = neg;
		union{
			pixel *a;
			int32_t *b;
		} a;
		a.a = &usedColor;
		*(a.b) = ~(*(a.b));
		//usedColor = a.a;
		usedColor.a = 0;
	}
}

void ScreenWriter::color(uint8_t r, uint8_t g, uint8_t b){
	//fprintf(fd.logfile, "Color set to (%d,%d,%d)\n", r,g,b);
	if(colorNegative){
		usedColor.r = ~r;
		usedColor.b = ~b;
		usedColor.g = ~g;
	}else{
		usedColor.r = r;
		usedColor.b = b;
		usedColor.g = g;
	}
}

void ScreenWriter::color(uint8_t greyScale){
	color(greyScale,greyScale,greyScale);
}

/*
void ScreenWriter::drawPixel(uint16_t x, uint16_t y, uint16_t dimIncrease = 0){
	checkinit();
	if(dimIncrease > 0){
		int16_t startx, starty, stopx, stopy;
		startx = x - dimIncrease;
		starty = y - dimIncrease;
		stopx = x + dimIncrease;
		stopy = y + dimIncrease;
		
		if(startx < 0){
			startx = 0;
		}
		
		if(stopx < startx){
			return;
		}

		if(stopx >= width){
			stopx = width -1;
		}
		
		if(startx > stopx){
			return;
		}
		
		if(starty < 0){
			starty = 0;
		}
		
		if(stopy < starty){
			return;
		}

		if(stopy >= width){
			stopy = width -1;
		}
		
		if(starty > stopy){
			return;
		}
		
		for(int i = starty; i <= stopy; i++){
			memset(nextBuffer + startx + width*i, *((int32_t*)&usedColor), sizeof(pixel) * (stopx - startx + 1));
		}
	}else{
		if(x >= width || y >= height){
			//fprintf(fd.logfile, "Not drawing at (%d,%d)\n", x , y);
			return;
		}
		memcpy(nextBuffer + x + width * y, &usedColor, sizeof(pixel));
	}
}
*/

void ScreenWriter::drawPixel(uint16_t x, uint16_t y){
	checkInit();
	if(x >= width || y >= height){
		return;
	}
	memcpy(nextBuffer + x + width * y, &usedColor, sizeof(pixel));

}

void ScreenWriter::rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height){
	checkInit();
	for(int i = 0; i < width; i++){
		for(int j= 0; j < height; j++){
			drawPixel(x+i, y+j);
		}
	}
}
void ScreenWriter::background(uint8_t r,uint8_t g,uint8_t b){
	checkInit();
	pixel f;
	if(colorNegative){
		f.r = ~r;
		f.g = ~g;
		f.b = ~b;
	}else{
		f.r = r;
		f.g = g;
		f.b = b;
	}
	for(pixel* i = nextBuffer; i < nextBuffer + width*height; i++){
		memcpy(i, &f, sizeof(pixel));
	}
	//memset(nextBuffer, *((uint32_t*)&f), size);
}

void ScreenWriter::background(uint8_t greyScale){
	background(greyScale,greyScale,greyScale);
}

/*
void ScreenWriter::line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t dimIncrease = 0){
	checkinit();
	double slope = (double)(y1 - y0) / (x1 - x0);
	if(slope <= 1 && slope >= -1){
		//work with x
		if(x1 < x0){
			x1^=x0;
			x0^=x1;
			x1^=x0;
			y1^=y0;
			y0^=y1;
			y1^=y0;
		}
		
		for(int i = 0; i <= x1 - x0 ; i++){
			drawPixel(x0 + i, y0 + i * slope, dimIncrease);
		}
	}else{
		slope = (double)(x1 - x0) / (y1 - y0);
		//work with y
		if(y1 < y0){
			x1^=x0;
			x0^=x1;
			x1^=x0;
			y1^=y0;
			y0^=y1;
			y1^=y0;
		}
		
		for(int i = 0; i <= y1 - y0 ; i++){
			drawPixel(x0 + i * slope, y0 + i, dimIncrease);
		}
	}
}
*/

void ScreenWriter::line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	checkInit();
	double slope = (double)(y1 - y0) / (x1 - x0);
	if(slope <= 1 && slope >= -1){
		//work with x
		if(x1 < x0){
			x1^=x0;
			x0^=x1;
			x1^=x0;
			y1^=y0;
			y0^=y1;
			y1^=y0;
		}
		
		for(int i = 0; i <= x1 - x0 ; i++){
			drawPixel(x0 + i, y0 + i * slope);
		}
	}else{
		slope = (double)(x1 - x0) / (y1 - y0);
		//work with y
		if(y1 < y0){
			x1^=x0;
			x0^=x1;
			x1^=x0;
			y1^=y0;
			y0^=y1;
			y1^=y0;
		}
		
		for(int i = 0; i <= y1 - y0 ; i++){
			drawPixel(x0 + i * slope, y0 + i);
		}
	}
}
