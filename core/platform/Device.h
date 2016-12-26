#ifndef __CCDEVICE_H__
#define __CCDEVICE_H__

struct FontDefinition;

class Device
{
public:
	enum class TextAlign
	{
		CENTER = 0x33, ///< Horizontal center and vertical center.
		TOP = 0x13, ///< Horizontal center and vertical top.
		TOP_RIGHT = 0x12, ///< Horizontal right and vertical top.
		RIGHT = 0x32, ///< Horizontal right and vertical center.
		BOTTOM_RIGHT = 0x22, ///< Horizontal right and vertical bottom.
		BOTTOM = 0x23, ///< Horizontal center and vertical bottom.
		BOTTOM_LEFT = 0x21, ///< Horizontal left and vertical bottom.
		LEFT = 0x31, ///< Horizontal left and vertical center.
		TOP_LEFT = 0x11, ///< Horizontal left and vertical top.
	};
	/**
	*  Gets the DPI of device
	*  @return The DPI of device.
	*/
	static int getDPI();

	/**
	* To enable or disable accelerometer.
	*/
	static void setAccelerometerEnabled(bool isEnabled);
	/**
	*  Sets the interval of accelerometer.
	*/
	static void setAccelerometerInterval(float interval);

	static unsigned char *getTextureDataForText(const char * text, const FontDefinition& textDefinition, TextAlign align, int &width, int &height, bool& hasPremultipliedAlpha);
};


#endif /* __CCDEVICE_H__ */
