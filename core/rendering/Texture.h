#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <gles_include.h>
#include <rendering/types.h>

enum class PixelFormat
{
	//! auto detect the type
	AUTO,
	//! 32-bit texture: BGRA8888
	BGRA8888,
	//! 32-bit texture: RGBA8888
	RGBA8888,
	//! 24-bit texture: RGBA888
	RGB888,
	//! 16-bit texture without Alpha channel
	RGB565,
	//! 8-bit textures used as masks
	A8,
	//! 8-bit intensity texture
	I8,
	//! 16-bit textures used as masks
	AI88,
	//! 16-bit textures: RGBA4444
	RGBA4444,
	//! 16-bit textures: RGB5A1
	RGB5A1,
	//! 4-bit PVRTC-compressed texture: PVRTC4
	PVRTC4,
	//! 4-bit PVRTC-compressed texture: PVRTC4 (has alpha channel)
	PVRTC4A,
	//! 2-bit PVRTC-compressed texture: PVRTC2
	PVRTC2,
	//! 2-bit PVRTC-compressed texture: PVRTC2 (has alpha channel)
	PVRTC2A,
	//! ETC-compressed texture: ETC
	ETC,
	//! S3TC-compressed texture: S3TC_Dxt1
	S3TC_DXT1,
	//! S3TC-compressed texture: S3TC_Dxt3
	S3TC_DXT3,
	//! S3TC-compressed texture: S3TC_Dxt5
	S3TC_DXT5,
	//! ATITC-compressed texture: ATC_RGB
	ATC_RGB,
	//! ATITC-compressed texture: ATC_EXPLICIT_ALPHA
	ATC_EXPLICIT_ALPHA,
	//! ATITC-compresed texture: ATC_INTERPOLATED_ALPHA
	ATC_INTERPOLATED_ALPHA,
	//! Default texture format: AUTO
	DEFAULT = AUTO,

	NONE = -1
};

struct PixelFormatInfo {

	PixelFormatInfo(GLenum anInternalFormat, GLenum aFormat, GLenum aType, int aBpp, bool aCompressed, bool anAlpha)
	: internalFormat(anInternalFormat)
	, format(aFormat)
	, type(aType)
	, bpp(aBpp)
	, compressed(aCompressed)
	, alpha(anAlpha)
	{}

	GLenum internalFormat;
	GLenum format;
	GLenum type;
	int bpp;
	bool compressed;
	bool alpha;
};

class Texture
{
public:
	Texture();
	~Texture();

	bool initWithData(const void *data, size_t dataLen, PixelFormat pixelFormat, int pixelsWide, int pixelsHigh, const Size& contentSize);
	/** Initializes a texture from a string with dimensions, alignment, font name and font size */
	bool initWithString(const char *text, const std::string &fontName, float fontSize, const Size& dimensions = Size(0, 0), TextHAlignment hAlignment = TextHAlignment::CENTER, TextVAlignment vAlignment = TextVAlignment::TOP);
	/** Initializes a texture from a string using a text definition*/
	bool initWithString(const char *text, const FontDefinition& textDefinition);

	/**
	Convert the format to the format param you specified, if the format is PixelFormat::Automatic, it will detect it automatically and convert to the closest format for you.
	It will return the converted format to you. if the outData != data, you must delete it manually.
	*/
	static PixelFormat convertDataToFormat(const unsigned char* data, size_t dataLen, PixelFormat originFormat, PixelFormat format, unsigned char** outData, size_t* outDataLen);
	static PixelFormat convertRGBA8888ToFormat(const unsigned char* data, size_t dataLen, PixelFormat format, unsigned char** outData, size_t* outDataLen);

	GLuint getTextureId();

private:
	GLuint m_textureId;
};

#endif __TEXTURE_H__