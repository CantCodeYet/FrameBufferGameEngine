#include <cstdint>
#include "../lib/closeable.hpp"

#define fbdev "/dev/fb0"
#define ttydev "/dev/tty"
#define MOD_NON 0 //no set mode
#define MOD_TXT 1 //text mode
#define MOD_GRA 2 //graphic mode

using namespace std;

typedef struct{
	uint8_t b, g, r, a;
} pixel;

class ScreenWriter : public Closeable{
private:
	bool initialized;
	pixel usedColor;
	uint8_t ttyMode;
	bool colorNegative;
	struct{
		int framebuffer;
		int tty;
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
	
	void background(uint8_t r, uint8_t g, uint8_t b); //draws the background using the defined color
	
	void background(uint8_t greyScale); //draws the background using the defined color
	
	void show();//stops the console from displaying and enables to draw on the screen
		
	void hide();//set console to text mode and stop drawing
	
	void init();//prepare the screen writer for writing
	
	void onClose();//close the screenWriter
	
	void applyBuffer();//draws the prepared buffer to the actual buffer

	void negative();//negative effect
	
	void negative(bool);//negative effect
	
	void rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height); //draws a rectangle

	void drawPixel(uint16_t x, uint16_t y); //draws a pixel on the screen
	
	void color(uint8_t r, uint8_t g, uint8_t b); //sets the color used to draw
	
	void color(uint8_t greyScale); //sets the color used to draw

	void line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1); //draws a line from (x0,y0) to (x1,y1)
};