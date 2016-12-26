#ifndef __TYPES__
#define __TYPES__

#include <string>


enum class TextVAlignment
{
	TOP,
	CENTER,
	BOTTOM,
};

enum class TextHAlignment
{
	LEFT,
	CENTER,
	RIGHT,
};

struct Size
{
	float width;
	float height;

	Size(float w, float h)
	{
		width = w;
		height = h;
	}
};

struct Color3B
{
	unsigned char r;
	unsigned char g;
	unsigned char b;

	Color3B(unsigned char red, unsigned char green, unsigned char blue)
	{
		r = red;
		g = green;
		b = blue;
	}
};

// font attributes
struct FontDefinition
{
public:
	/**
	* @js NA
	* @lua NA
	*/
	FontDefinition()
		: _fontSize(0)
		, _alignment(TextHAlignment::CENTER)
		, _vertAlignment(TextVAlignment::TOP)
		, _dimensions(Size(0,0))
		, _fontFillColor(Color3B(255,255,255))
	{}

	// font name
	std::string           _fontName;
	// font size
	int                   _fontSize;
	// horizontal alignment
	TextHAlignment        _alignment;
	// vertical alignment
	TextVAlignment _vertAlignment;
	// renering box
	Size                  _dimensions;
	// font color
	Color3B               _fontFillColor;
};

#if !defined(_DEBUG)
#define CHECK_GL_ERROR_DEBUG()
#else
#define CHECK_GL_ERROR_DEBUG() \
	do { \
	\
		GLenum __error = glGetError(); \
		if (__error) {\
			printf("OpenGL error 0x%04X in %s %s %d\n", __error, __FILE__, __FUNCTION__, __LINE__); \
		} \
	} while (false)
#endif

#endif __TYPES__