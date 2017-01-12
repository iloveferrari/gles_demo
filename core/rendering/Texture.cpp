#include "Texture.h"
#include <platform/Device.h>


Texture::Texture()
	:m_textureId(0)
{

}

Texture::~Texture()
{
	if (m_textureId)
	{
		glDeleteTextures(1, &m_textureId);
	}
}

bool Texture::initWithString(const char *text, const std::string& fontName, float fontSize, const Size& dimensions/* = Size(0, 0)*/, TextHAlignment hAlignment/* =  TextHAlignment::CENTER */, TextVAlignment vAlignment/* =  TextVAlignment::TOP */)
{
	FontDefinition tempDef;

	tempDef._fontName = fontName;
	tempDef._fontSize = fontSize;
	tempDef._dimensions = dimensions;
	tempDef._alignment = hAlignment;
	tempDef._vertAlignment = vAlignment;
	tempDef._fontFillColor = Color3B(1.0f, 1.0f, 1.0f);

	return initWithString(text, tempDef);
}

bool Texture::initWithString(const char *text, const FontDefinition& textDefinition)
{
	if (!text || 0 == strlen(text))
	{
		return false;
	}

	bool ret = false;
	Device::TextAlign align;

	if (TextVAlignment::TOP == textDefinition._vertAlignment)
	{
		align = (TextHAlignment::CENTER == textDefinition._alignment) ? Device::TextAlign::TOP
			: (TextHAlignment::LEFT == textDefinition._alignment) ? Device::TextAlign::TOP_LEFT : Device::TextAlign::TOP_RIGHT;
	}
	else if (TextVAlignment::CENTER == textDefinition._vertAlignment)
	{
		align = (TextHAlignment::CENTER == textDefinition._alignment) ? Device::TextAlign::CENTER
			: (TextHAlignment::LEFT == textDefinition._alignment) ? Device::TextAlign::LEFT : Device::TextAlign::RIGHT;
	}
	else if (TextVAlignment::BOTTOM == textDefinition._vertAlignment)
	{
		align = (TextHAlignment::CENTER == textDefinition._alignment) ? Device::TextAlign::BOTTOM
			: (TextHAlignment::LEFT == textDefinition._alignment) ? Device::TextAlign::BOTTOM_LEFT : Device::TextAlign::BOTTOM_RIGHT;
	}
	else
	{
		assert(false, "Not supported alignment format!");
		return false;
	}

	PixelFormat pixelFormat = PixelFormat::DEFAULT;
	unsigned char* outTempData = nullptr;
	size_t outTempDataLen = 0;

	int imageWidth;
	int imageHeight;
	auto textDef = textDefinition;
	auto contentScaleFactor = 1.0f;//CC_CONTENT_SCALE_FACTOR();
	textDef._fontSize *= contentScaleFactor;
	textDef._dimensions.width *= contentScaleFactor;
	textDef._dimensions.height *= contentScaleFactor;

	bool hasPremultipliedAlpha;
	unsigned char *buffer = Device::getTextureDataForText(text, textDef, align, imageWidth, imageHeight, hasPremultipliedAlpha);
	if (buffer == nullptr)
	{
		return false;
	}

	Size  imageSize = Size((float)imageWidth, (float)imageHeight);
	pixelFormat = convertDataToFormat(buffer, imageWidth*imageHeight * 4, PixelFormat::RGBA8888, pixelFormat, &outTempData, &outTempDataLen);

	ret = initWithData(outTempData, outTempDataLen, pixelFormat, imageWidth, imageHeight, imageSize);

	if (buffer != nullptr)
	{
		free(buffer);
	}

	//_hasPremultipliedAlpha = hasPremultipliedAlpha;

	return ret;
}

PixelFormat Texture::convertDataToFormat(const unsigned char* data, size_t dataLen, PixelFormat originFormat, PixelFormat format, unsigned char** outData, size_t* outDataLen)
{
	// don't need to convert
	if (format == originFormat || format == PixelFormat::AUTO)
	{
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return originFormat;
	}

	switch (originFormat)
	{
	//case PixelFormat::I8:
	//	return convertI8ToFormat(data, dataLen, format, outData, outDataLen);
	//case PixelFormat::AI88:
	//	return convertAI88ToFormat(data, dataLen, format, outData, outDataLen);
	//case PixelFormat::RGB888:
	//	return convertRGB888ToFormat(data, dataLen, format, outData, outDataLen);
	case PixelFormat::RGBA8888:
		return convertRGBA8888ToFormat(data, dataLen, format, outData, outDataLen);
	default:
		printf("unsupport convert for format %d to format %d", originFormat, format);
		*outData = (unsigned char*)data;
		*outDataLen = dataLen;
		return originFormat;
	}
}

bool Texture::initWithData(const void *data, size_t dataLen, PixelFormat pixelFormat, int pixelsWide, int pixelsHigh, const Size& contentSize)
{
	assert(dataLen>0 && pixelsWide>0 && pixelsHigh>0, "Invalid size");

	//the pixelFormat must be a certain value 
	assert(pixelFormat != PixelFormat::NONE && pixelFormat != PixelFormat::AUTO, "the \"pixelFormat\" param must be a certain value!");
	assert(pixelsWide>0 && pixelsHigh>0, "Invalid size");

	const PixelFormatInfo& info = PixelFormatInfo(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 32, false, true);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &m_textureId);
	glBindTexture(GL_TEXTURE_2D, m_textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	CHECK_GL_ERROR_DEBUG(); // clean possible GL error

	// Specify OpenGL texture image
	int width = pixelsWide;
	int height = pixelsHigh;

	if (info.compressed)
	{
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, (GLsizei)width, (GLsizei)height, 0, dataLen, data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, info.internalFormat, (GLsizei)width, (GLsizei)height, 0, info.format, info.type, data);
	}


	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		printf("Texture: Error uploading texture, glError: 0x%04X", err);
		return false;
	}

	return true;
}

GLuint Texture::getTextureId()
{
	return m_textureId;
}

PixelFormat Texture::convertRGBA8888ToFormat(const unsigned char* data, size_t dataLen, PixelFormat format, unsigned char** outData, size_t* outDataLen)
{
	*outData = (unsigned char*)data;
	*outDataLen = dataLen;
	return PixelFormat::RGBA8888;
}