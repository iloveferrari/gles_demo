#ifndef __SKY__
#define __SKY__


#include <gles_include.h>
#include <SkyModel.h>
#include <memory>

class Sky
{
public:
	Sky();
	~Sky();

	bool init();
	void InitModel();
	void draw(ESContext *esContext);

	std::string getStringFromFile(const char* filename);

private:
	float m_radius;
	int m_numIndices;

	GLint m_textureLoc;
	GLint m_mvpLoc;

	float m_theta;

	float m_Kr, m_Kr4PI;
	float m_Km, m_Km4PI;
	float m_ESun;
	float m_g;
	float m_fExposure;

	float m_fRayleighScaleDepth;
	float m_fMieScaleDepth;

	float m_fInnerRadius;
	float m_fOuterRadius;

	float m_fWavelength[3];
	float m_fWavelength4[3];

	GLuint m_indicesVBO;
	GLuint m_verticesVBO;
	GLuint m_normalsVBO;
	GLuint m_texCoordsVBO;

	GLuint m_textureId;
	GLuint m_program;

	bool use_constant_solar_spectrum_;
	bool use_combined_textures_;
	bool use_luminance_;
	bool do_white_balance_;
	bool show_help_;

	std::unique_ptr<SkyModel> model_;
	unsigned int program_;
	int window_id_;

	double view_distance_meters_;
	double view_zenith_angle_radians_;
	double view_azimuth_angle_radians_;
	double sun_zenith_angle_radians_;
	double sun_azimuth_angle_radians_;
	double exposure_;

	int previous_mouse_x_;
	int previous_mouse_y_;
	bool is_ctrl_key_pressed_;
};

#endif