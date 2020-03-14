/*
 * @author Spikatrix
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <png.h>

#define SCREENSHOT_FILENAME "tempScreenshot.png"

// Point structure
struct p
{
	int x, y;
};

// Color structure
struct c 
{
	int r, g, b, a;
};

void get_color_from_png_file(char *filename, struct p* clickPoint, struct c* color) {
	FILE *fp = fopen(filename, "rb");

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) abort();

	png_infop info = png_create_info_struct(png);
	if(!info) abort();

	if(setjmp(png_jmpbuf(png))) abort();

	png_init_io(png, fp);

	png_read_info(png, info);

	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth  = png_get_bit_depth(png, info);
	png_bytep* row_pointers = NULL;

	// Read any color_type into 8bit depth, RGBA format.
	// See http://www.libpng.org/pub/png/libpng-manual.txt

	if(bit_depth == 16)
		png_set_strip_16(png);

	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if(color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if(color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	if (row_pointers) abort();

	row_pointers = malloc(sizeof(png_bytep) * height);
	for(int y = 0; y < height; y++) {
		row_pointers[y] = malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, row_pointers);

	fclose(fp);

	png_destroy_read_struct(&png, &info, NULL);

	png_bytep px = &(row_pointers[clickPoint -> y][clickPoint -> x * 4]);
	color -> r = px[0];
	color -> g = px[1];
	color -> b = px[2];
	color -> a = px[3];

	for (int i = 0; i < height; i++)
	{
		free(row_pointers[i]);
	}
	free(row_pointers);
}

void cleanup(Display*);

Display* get_display()
{
	Display* display = XOpenDisplay(NULL); 
	if (!display)
	{
		fputs("ERROR: Failed to open the display!\n", stderr);
		exit(EXIT_FAILURE);
	}

	return display;
}

Window get_root_window(Display* display)
{
	return RootWindow(display, XDefaultScreen(display));
}

Cursor get_cross_cursor(Display* display)
{
	Cursor cursor = XCreateFontCursor(display, XC_tcross);
	if (!cursor)
	{
		fputs("ERROR: Failed to create Cross Cursor!\n", stderr);
		cleanup(display);
		exit(EXIT_FAILURE);
	}

	return cursor;
}

struct p* initiate_events(Display* display, Window root_window)
{
	XEvent event;	

	XAllowEvents(display, SyncPointer, CurrentTime);
	XWindowEvent(display, root_window, ButtonPressMask, &event);	

	if (event.type == ButtonPress)
	{
		struct p* point = malloc(1 * sizeof *point);
		if (!point)
		{
			fputs("ERROR: Failed to allocate memory for storing the click location!\n", stderr);
			cleanup(display);
			exit(EXIT_FAILURE);
		}

		point -> x = event.xbutton.x;
		point -> y = event.xbutton.y;

		//printf("X: %d, Y: %d\n", point -> x, point -> y);

		return point;
	}

	return NULL;
}

void grab_pointer(Display* display, Window root_window, Cursor cursor)
{
	int status = XGrabPointer(display, root_window, false,
			(unsigned int) ButtonPressMask, GrabModeSync,
			GrabModeAsync, root_window, cursor, CurrentTime);
	if (status == GrabSuccess)
	{
		puts("Click on any pixel on your screen to display its color");
		return; 
	}
	else
	{
		fputs("ERROR: Failed to grab the pointer!\n", stderr);
		cleanup(display);
		exit(EXIT_FAILURE);
	}
}

void save_screenshot()
{
	system("scrot '" SCREENSHOT_FILENAME "' -q 100");
	//system("gnome-screenshot -f '" SCREENSHOT_FILENAME "' -d 0");
	//Or any other screenshot tool
}

void delete_screenshot()
{
	system("rm " SCREENSHOT_FILENAME);
}

int main(void)
{
	Display* display = get_display();
	Window root_window = get_root_window(display);
	Cursor cursor = get_cross_cursor(display);

	grab_pointer(display, root_window, cursor);
	struct p* clickPoint = initiate_events(display, root_window);

	if (!clickPoint)
	{
		fputs("ERROR: Failed to get the click point location!\n", stderr);
		cleanup(display);
		return EXIT_FAILURE;
	}

	struct c color;

	save_screenshot();
	get_color_from_png_file(SCREENSHOT_FILENAME, clickPoint, &color);

	printf("(%d, %d) = RGB(%d, %d, %d) = #%02X%02X%02X\n",
			clickPoint -> x, clickPoint -> y, 
			color.r, color.g, color.b,
			color.r, color.g, color.b);

	delete_screenshot();

	free(clickPoint);
	cleanup(display);
	return EXIT_SUCCESS;
}

void cleanup(Display* display)
{
	XCloseDisplay(display);
}
