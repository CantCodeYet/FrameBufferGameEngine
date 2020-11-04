#include "FrameBufferGraphics.hpp"
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstring>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <stdexcept>

#define checkInit() \
if(!initialized){\
	throw new runtime_error("ScreenWriter not initialized or already finalized. Initialize again to use.");\
}


ScreenWriter::ScreenWriter() :
	initialized(false),
	colorNegative(false),
	fd({-1,-1}),
	buffer(nullptr),
	nextBuffer(nullptr),
	usedColor({0}){
	init();
}

ScreenWriter::~ScreenWriter(){
	onClose();
}

void ScreenWriter::init(){
	if(initialized)
		return;
	//get terminal configuration
	fd.tty = open(ttydev, O_RDWR);
	if(fd.tty < 0){
		throw new runtime_error( "Cannot access tty device");
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
		onClose();
		throw new runtime_error( "Cannot access framebuffer device");
	}
	
	fb_fix_screeninfo fix;
	fb_var_screeninfo var;
	
	
	if(ioctl(fd.framebuffer, FBIOGET_FSCREENINFO, &fix) == -1){
		onClose();
		throw new runtime_error( "Cannot access framebuffer fixed info");
	} 
	
	
	if(ioctl(fd.framebuffer, FBIOGET_VSCREENINFO, &var) == -1){
		onClose();
		throw new runtime_error( "Cannot access framebuffer variable info");
	}
	
	width = var.xres;
	height = var.yres;
	size = width * height * sizeof(pixel);
	
	//map framebuffer to buffer
	buffer = (pixel*)mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.framebuffer, 0);
	
	
	if(buffer == MAP_FAILED){
		onClose();
		throw new runtime_error( "Cannot map framebuffer to memory");
	}
	
	nextBuffer = (pixel*)malloc(size);
	if(nextBuffer == NULL){
		onClose();
		throw new runtime_error( "Cannot create prepared buffer");
	}
	
	memset(nextBuffer, 0, size);
	
	initialized = true;
}

void ScreenWriter::show(){
	checkInit();
	if(ttyMode != MOD_GRA){	
		if(ioctl(fd.tty, KDSETMODE, KD_GRAPHICS) == -1){
			throw new runtime_error( "Cannot set graphic mode");
		}
		ttyMode = MOD_GRA;
	}
}

void ScreenWriter::hide(){
	checkInit();
	if(ttyMode != MOD_TXT){	
		if(ioctl(fd.tty, KDSETMODE, KD_TEXT) == -1){
			throw new runtime_error( "Cannot set graphic mode");
		}
		ttyMode = MOD_TXT;
	}
}

void ScreenWriter::onClose(){
	if(fd.tty != -1){
		hide();
		if(close(fd.tty) == -1){
			throw new runtime_error( "Unable to close tty file descriptor" );
		}
		fd.tty = -1;
	}
	
	if(buffer != NULL && buffer != MAP_FAILED){
		if(munmap(buffer, size) == -1){
			throw new runtime_error( "Cannot unmap framebuffer from memory" );
		}
		buffer = NULL;
	}
	
	if(fd.framebuffer != -1){
		if(close(fd.framebuffer) == -1){
			throw new runtime_error( "Unable to close framebuffer file descriptor" );
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

void ScreenWriter::applyBuffer(){
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
}

void ScreenWriter::background(uint8_t greyScale){
	background(greyScale,greyScale,greyScale);
}

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
