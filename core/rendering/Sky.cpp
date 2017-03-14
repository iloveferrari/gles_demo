#include "Sky.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#define POSITION_LOC    0
#define TEXCOORD_LOC    1

#define M_PI 3.1415926535897f

const double kSunAngularRadius = 0.00935 / 2.0;
const double kSunSolidAngle = 2.0 * M_PI * (1.0 - cos(kSunAngularRadius));
const double kLengthUnitInMeters = 1000.0;

Sky::Sky():
	use_constant_solar_spectrum_(false),
	use_combined_textures_(true),
	use_luminance_(true),
	do_white_balance_(false),
	show_help_(true),
	program_(0),
	view_distance_meters_(9000.0),
	view_zenith_angle_radians_(1.47),
	view_azimuth_angle_radians_(0.1),
	sun_zenith_angle_radians_(1.3),
	sun_azimuth_angle_radians_(2.9),
	exposure_(10.0)
{
	m_theta = 5.0f;
}

Sky::~Sky()
{

}

bool Sky::init()
{
	InitModel();

	return true;
}

void Sky::InitModel() 
{
	// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
	// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
	// summed and averaged in each bin (e.g. the value for 360nm is the average
	// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
	const int kLambdaMin = 360;
	const int kLambdaMax = 830;
	const double kSolarIrradiance[48] = 
	{
		1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
		1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
		1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
		1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
		1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
		1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
	};

	const char* kVertexShader = 
	     R"(#version 300 es
			uniform mat4 model_from_view;
			uniform mat4 view_from_clip;
			layout(location = 0) in vec4 vertex;
			out vec3 view_ray;
			void main() 
			{
				view_ray = (model_from_view * vec4((view_from_clip * vertex).xyz, 0.0)).xyz;
				gl_Position = vertex;
			})";

	// Wavelength independent solar irradiance "spectrum" (not physically
	// realistic, but was used in the original implementation).
	const double kConstantSolarIrradiance = 1.5;
	const double kBottomRadius = 6360000.0;
	const double kTopRadius = 6420000.0;
	const double kRayleigh = 1.24062e-6;
	const double kRayleighScaleHeight = 8000.0;
	const double kMieScaleHeight = 1200.0;
	const double kMieAngstromAlpha = 0.0;
	const double kMieAngstromBeta = 5.328e-3;
	const double kMieSingleScatteringAlbedo = 0.9;
	const double kMiePhaseFunctionG = 0.8;
	const double kGroundAlbedo = 0.1;
	const double kMaxSunZenithAngle = 102.0 / 180.0 * M_PI;

	std::vector<double> wavelengths;
	std::vector<double> solar_irradiance;
	std::vector<double> rayleigh_scattering;
	std::vector<double> mie_scattering;
	std::vector<double> mie_extinction;
	std::vector<double> ground_albedo;
	for (int l = kLambdaMin; l <= kLambdaMax; l += 10) 
	{
		double lambda = static_cast<double>(l)* 1e-3;  // micro-meters
		double mie =
			kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
		wavelengths.push_back(l);
		if (use_constant_solar_spectrum_) {
			solar_irradiance.push_back(kConstantSolarIrradiance);
		}
		else {
			solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
		}
		rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
		mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
		mie_extinction.push_back(mie);
		ground_albedo.push_back(kGroundAlbedo);
	}
	CHECK_GL_ERROR_DEBUG();
	model_.reset(new SkyModel(wavelengths, solar_irradiance, kSunAngularRadius,
		kBottomRadius, kTopRadius, kRayleighScaleHeight, rayleigh_scattering,
		kMieScaleHeight, mie_scattering, mie_extinction, kMiePhaseFunctionG,
		ground_albedo, kMaxSunZenithAngle, kLengthUnitInMeters,
		use_combined_textures_));
	model_->Init();
	CHECK_GL_ERROR_DEBUG();
	/*
	<p>Then, it creates and compiles the vertex and fragment shaders used to render
	our demo scene, and link them with the <code>Model</code>'s atmosphere shader
	to get the final scene rendering program:
	*/
	CHECK_GL_ERROR_DEBUG();
	GLuint vertex_shader = esLoadShader(GL_VERTEX_SHADER, kVertexShader);
	CHECK_GL_ERROR_DEBUG();
	const std::string fragment_shader_str =
		model_->getAtmosphereShaderStr() +
		std::string(use_luminance_ ? "\n#define USE_LUMINANCE\n" : "") +
		getStringFromFile("core/demo.c");

	const char* fragment_shader_source = fragment_shader_str.c_str();
	GLuint fragment_shader = esLoadShader(GL_FRAGMENT_SHADER, fragment_shader_source);
	CHECK_GL_ERROR_DEBUG();
	if (program_ != 0) {
		glDeleteProgram(program_);
	}
	program_ = glCreateProgram();
	CHECK_GL_ERROR_DEBUG();
	glAttachShader(program_, vertex_shader);
	CHECK_GL_ERROR_DEBUG();
	glAttachShader(program_, fragment_shader);
	CHECK_GL_ERROR_DEBUG();
	glLinkProgram(program_);
	CHECK_GL_ERROR_DEBUG();
	GLint linked;
	// Check the link status
	glGetProgramiv(program_, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;

		glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char *infoLog = (char *)malloc(sizeof(char)* infoLen);

			glGetProgramInfoLog(program_, infoLen, NULL, infoLog);
			esLogMessage("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(program_);
		//return 0;
	}
	glDetachShader(program_, vertex_shader);
	glDetachShader(program_, fragment_shader);
	//glDetachShader(program_, model_->GetShader());
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	CHECK_GL_ERROR_DEBUG();
	/*
	<p>Finally, it sets the uniforms of this program that can be set once and for
	all (in our case this includes the <code>Model</code>'s texture uniforms,
	because our demo app does not have any texture of its own):
	*/
	CHECK_GL_ERROR_DEBUG();
	glUseProgram(program_);
	CHECK_GL_ERROR_DEBUG();
	model_->SetProgramUniforms(program_, 0, 1, 2, 3);
	double white_point_r = 1.0;
	double white_point_g = 1.0;
	double white_point_b = 1.0;
	if (do_white_balance_) {
		SkyModel::ConvertSpectrumToLinearSrgb(wavelengths, solar_irradiance,
			&white_point_r, &white_point_g, &white_point_b);
		double white_point = (white_point_r + white_point_g + white_point_b) / 3.0;
		white_point_r /= white_point;
		white_point_g /= white_point;
		white_point_b /= white_point;
	}
	glUniform3f(glGetUniformLocation(program_, "white_point"),
		white_point_r, white_point_g, white_point_b);
	glUniform3f(glGetUniformLocation(program_, "earth_center"),
		0.0, 0.0, -kBottomRadius / kLengthUnitInMeters);
	glUniform3f(glGetUniformLocation(program_, "sun_radiance"),
		kSolarIrradiance[0] / kSunSolidAngle,
		kSolarIrradiance[1] / kSunSolidAngle,
		kSolarIrradiance[2] / kSunSolidAngle);
	glUniform2f(glGetUniformLocation(program_, "sun_size"),
		tan(kSunAngularRadius),
		cos(kSunAngularRadius));
	CHECK_GL_ERROR_DEBUG();
}

std::string Sky::getStringFromFile(const char* filename)
{
	std::ifstream ifile(filename);
	//将文件读入到ostringstream对象buf中
	std::ostringstream buf;
	char ch;
	while (buf&&ifile.get(ch))
	{
		buf.put(ch);
	}
	//返回与流对象buf关联的字符串
	return buf.str();
}

void Sky::draw(ESContext *esContext)
{
	CHECK_GL_ERROR_DEBUG();
	glUseProgram(program_);
	CHECK_GL_ERROR_DEBUG();
	const float kFovY = 50.0 / 180.0 * M_PI;
	const float kTanFovY = tan(kFovY / 2.0);
	float aspect_ratio = static_cast<float>(esContext->width) / esContext->height;

	// Transform matrix from clip space to camera space.
	float view_from_clip[16] = {
		kTanFovY * aspect_ratio, 0.0, 0.0, 0.0,
		0.0, kTanFovY, 0.0, 0.0,
		0.0, 0.0, 0.0, -1.0,
		0.0, 0.0, 1.0, 1.0
	};
	glUniformMatrix4fv(glGetUniformLocation(program_, "view_from_clip"), 1, true,
		view_from_clip);

	glUniform3f(glGetUniformLocation(program_, "camera"),
		esContext->camera_pos.x,
		esContext->camera_pos.y,
		esContext->camera_pos.z);
	glUniform1f(glGetUniformLocation(program_, "exposure"),
		use_luminance_ ? exposure_ * 1e-5 : exposure_);
	glUniformMatrix4fv(glGetUniformLocation(program_, "model_from_view"),
		1, true, &esContext->mvp_matrix[0][0]);
	glUniform3f(glGetUniformLocation(program_, "sun_direction"),
		cos(sun_azimuth_angle_radians_) * sin(sun_zenith_angle_radians_),
		sin(sun_azimuth_angle_radians_) * sin(sun_zenith_angle_radians_),
		cos(sun_zenith_angle_radians_));
	CHECK_GL_ERROR_DEBUG();
	GLfloat vertexPos[] =
	{
		-1.0, -1.0, 0.0, 1.0,   // v0
		+1.0, -1.0, 0.0, 1.0,   // v1
		-1.0, +1.0, 0.0, 1.0,   // v2
		+1.0, +1.0, 0.0, 1.0    // v3
	};

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertexPos);
	glEnableVertexAttribArray(0);

	CHECK_GL_ERROR_DEBUG();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	CHECK_GL_ERROR_DEBUG();

	glDisableVertexAttribArray(0);

	CHECK_GL_ERROR_DEBUG();

	glViewport(0, 0, esContext->width, esContext->height);
}
