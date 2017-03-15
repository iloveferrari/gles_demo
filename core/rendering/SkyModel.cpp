/**
* Copyright (c) 2017 Eric Bruneton
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holders nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/*<h2>atmosphere/model.cc</h2>

<p>This file implements the <a href="model.h.html">API of our atmosphere
model</a>. Its main role is to precompute the transmittance, scattering and
irradiance textures. The GLSL functions to precompute them are provided in
<a href="functions.glsl.html">functions.glsl</a>, but they are not sufficient.
They must be used in fully functional shaders and programs, and these programs
must be called in the correct order, with the correct input and output textures
(via framebuffer objects), to precompute each scattering order in sequence, as
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. This is the role
of the following C++ code.
*/

#include "SkyModel.h"

#include <gles_include.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

#include "constants.h"

/*
<p>The rest of this file is organized in 3 parts:
<ul>
<li>the <a href="#shaders">first part</a> defines the shaders used to precompute
the atmospheric textures,</li>
<li>the <a href="#utilities">second part</a> provides utility classes and
functions used to compile shaders, create textures, draw quads, etc,</li>
<li>the <a href="#implementation">third part</a> provides the actual
implementation of the <code>Model</code> class, using the above tools.</li>
</ul>

<h3 id="shaders">Shader definitions</h3>

<p>In order to precompute a texture we attach it to a framebuffer object (FBO)
and we render a full quad in this FBO. For this we need a basic vertex shader:
*/

const char* kVertexShader = 
R"( #version 300 es
    layout(location = 0) in vec2 vertex;
    void main() {
      gl_Position = vec4(vertex, 0.0, 1.0);
    })";

/*
<p>a basic geometry shader (only for 3D textures, to specify in which layer we
want to write):
*/

const char* kGeometryShader = R"(
    #version 300 es
    #extension GL_EXT_geometry_shader4 : enable
    layout(triangles) in;
    layout(triangle_strip, max_vertices = 3) out;
    uniform int layer;
    void main() {
      gl_Position = gl_PositionIn[0];
      gl_Layer = layer;
      EmitVertex();
      gl_Position = gl_PositionIn[1];
      gl_Layer = layer;
      EmitVertex();
      gl_Position = gl_PositionIn[2];
      gl_Layer = layer;
      EmitVertex();
      EndPrimitive();
    })";

/*
<p>and a fragment shader, which depends on the texture we want to compute. This
is the role of the following shaders, which simply wrap the precomputation
functions from <a href="functions.glsl.html">functions.glsl</a> in complete
shaders (with a <code>main</code> function and a proper declaration of the
shader inputs and outputs). Note that these strings must be concatenated with
<code>definitions.glsl</code> and <code>functions.glsl</code> (provided as C++
string literals by the generated <code>.glsl.inc</code> files), as well as with
a definition of the <code>ATMOSPHERE</code> constant - containing the atmosphere
parameters, to really get a complete shader:
*/

const char* kComputeTransmittanceShader = R"(
    layout(location = 0) out vec3 transmittance;
    void main() {
      transmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(
          ATMOSPHERE, gl_FragCoord.xy);
    })";

const char* kComputeDirectIrradianceShader = R"(
    layout(location = 0) out vec3 delta_irradiance;
    layout(location = 1) out vec3 irradiance;
    uniform sampler2D transmittance_texture;
    void main() {
      delta_irradiance = ComputeDirectIrradianceTexture(
          ATMOSPHERE, transmittance_texture, gl_FragCoord.xy);
      irradiance = vec3(0.0);
    })";

const char* kComputeSingleScatteringShader = R"(
    layout(location = 0) out vec3 delta_rayleigh;
    layout(location = 1) out vec3 delta_mie;
    layout(location = 2) out vec4 scattering;
    uniform sampler2D transmittance_texture;
    uniform float layer;
    void main() {
		ComputeSingleScatteringTexture(
			ATMOSPHERE, transmittance_texture, vec3(gl_FragCoord.xy, layer + 0.5),
			delta_rayleigh, delta_mie);
		scattering = vec4(delta_rayleigh.rgb, delta_mie.r);
    })";

const char* kComputeScatteringDensityShader = R"(
    layout(location = 0) out vec3 scattering_density;
    uniform sampler2D transmittance_texture;
    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;
    uniform sampler2D irradiance_texture;
    uniform int scattering_order;
    uniform float layer;
    void main() {
		scattering_density = ComputeScatteringDensityTexture(
			ATMOSPHERE, transmittance_texture, single_rayleigh_scattering_texture,
			single_mie_scattering_texture, multiple_scattering_texture,
			irradiance_texture, vec3(gl_FragCoord.xy, layer + 0.5),
			scattering_order);
    })";

const char* kComputeIndirectIrradianceShader = R"(
    layout(location = 0) out vec3 delta_irradiance;
    layout(location = 1) out vec3 irradiance;
    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;
    uniform int scattering_order;
    void main() {
		delta_irradiance = ComputeIndirectIrradianceTexture(
			ATMOSPHERE, single_rayleigh_scattering_texture,
			single_mie_scattering_texture, multiple_scattering_texture,
			gl_FragCoord.xy, scattering_order - 1);
		irradiance = delta_irradiance;
    })";

const char* kComputeMultipleScatteringShader = R"(
    layout(location = 0) out vec3 delta_multiple_scattering;
    layout(location = 1) out vec4 scattering;
    uniform sampler2D transmittance_texture;
    uniform sampler3D scattering_density_texture;
    uniform float layer;
    void main() {
		float nu;
		delta_multiple_scattering = ComputeMultipleScatteringTexture(
			ATMOSPHERE, transmittance_texture, scattering_density_texture,
			vec3(gl_FragCoord.xy, layer + 0.5), nu);
		scattering = vec4(
			delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);
    })";

/*
<p>We finally need a shader implementing the GLSL functions exposed in our API,
which can be done by calling the corresponding functions in functions.glsl,
with the precomputed texture arguments taken from uniform variables (note also the
_RADIANCE_TO_LUMINANCE conversion constants in the last functions:
they are computed in the second part below, and their definitions are
concatenated to this GLSL code to get a fully functional shader).
*/

const char* kAtmosphereShader = R"(
    uniform sampler2D transmittance_texture;
    uniform sampler3D scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler2D irradiance_texture;
    RadianceSpectrum GetSkyRadiance(
		Position camera, 
		Direction view_ray, 
		Length shadow_length,
		Direction sun_direction, 
		out DimensionlessSpectrum transmittance) 
	{
		return GetSkyRadiance(ATMOSPHERE, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, view_ray, shadow_length, sun_direction, transmittance);
    }

    RadianceSpectrum GetSkyRadianceToPoint(
        Position camera, Position point, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) 
	{
		return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture,
			scattering_texture, single_mie_scattering_texture,
			camera, point, shadow_length, sun_direction, transmittance);
    }

    IrradianceSpectrum GetSunAndSkyIrradiance(
		Position p, Direction normal, Direction sun_direction,
		out IrradianceSpectrum sky_irradiance) 
	{
		return GetSunAndSkyIrradiance(ATMOSPHERE, transmittance_texture,
			irradiance_texture, p, normal, sun_direction, sky_irradiance);
    }

    Luminance3 GetSkyLuminance(
        Position camera, Direction view_ray, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) 
	{
		return GetSkyRadiance(camera, view_ray, shadow_length, sun_direction,
			transmittance) * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }

    Luminance3 GetSkyLuminanceToPoint(
        Position camera, Position point, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) 
	{
		return GetSkyRadianceToPoint(camera, point, shadow_length, sun_direction,
			transmittance) * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }

    Illuminance3 GetSunAndSkyIlluminance(
		Position p, Direction normal, Direction sun_direction,
		out IrradianceSpectrum sky_irradiance) 
	{
		IrradianceSpectrum sun_irradiance = GetSunAndSkyIrradiance(p, normal, sun_direction, sky_irradiance);
		sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
		return sun_irradiance * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    })";

/*<h3 id="utilities">Utility classes and functions</h3>

<p>To compile and link these shaders into programs, and to set their uniforms,
we use the following utility class:
*/

class Program 
{
public:
	Program(
		const std::string& vertex_shader_source,
		const std::string& fragment_shader_source)
		: Program(vertex_shader_source, "", fragment_shader_source)
	{
	}

	Program(
		const std::string& vertex_shader_source,
		const std::string& geometry_shader_source,
		const std::string& fragment_shader_source)
	{
		program_ = glCreateProgram();

		const char* source;
		source = vertex_shader_source.c_str();
		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, &source, NULL);
		glCompileShader(vertex_shader);
		CheckShader(vertex_shader);
		glAttachShader(program_, vertex_shader);

		//GLuint geometry_shader = 0;
		//if (!geometry_shader_source.empty())
		//{
		//	source = geometry_shader_source.c_str();
		//	geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
		//	glShaderSource(geometry_shader, 1, &source, NULL);
		//	glCompileShader(geometry_shader);
		//	CheckShader(geometry_shader);
		//	glAttachShader(program_, geometry_shader);
		//}

		source = fragment_shader_source.c_str();
		GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, &source, NULL);
		glCompileShader(fragment_shader);
		CheckShader(fragment_shader);
		glAttachShader(program_, fragment_shader);

		glLinkProgram(program_);
		CheckProgram(program_);

		glDetachShader(program_, vertex_shader);
		glDeleteShader(vertex_shader);
		//if (!geometry_shader_source.empty()) 
		//{
		//	glDetachShader(program_, geometry_shader);
		//	glDeleteShader(geometry_shader);
		//}
		glDetachShader(program_, fragment_shader);
		glDeleteShader(fragment_shader);
	}

	~Program() 
	{
		glDeleteProgram(program_);
	}

	void Use() const 
	{
		glUseProgram(program_);
	}

	void BindInt(const std::string& uniform_name, int value) const 
	{
		glUniform1i(glGetUniformLocation(program_, uniform_name.c_str()), value);
	}

	void BindFloat(const std::string& uniform_name, float value) const
	{
		glUniform1f(glGetUniformLocation(program_, uniform_name.c_str()), value);
	}

	void BindTexture2d(const std::string& sampler_uniform_name, GLuint texture,
		GLuint texture_unit) const 
	{
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(GL_TEXTURE_2D, texture);
		BindInt(sampler_uniform_name, texture_unit);
	}

	void BindTexture3d(const std::string& sampler_uniform_name, GLuint texture,
		GLuint texture_unit) const 
	{
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(GL_TEXTURE_3D, texture);
		BindInt(sampler_uniform_name, texture_unit);
	}

private:
	static void CheckShader(GLuint shader) 
	{
		GLint compile_status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
		if (compile_status == GL_FALSE) 
		{
			PrintShaderLog(shader);
		}
		assert(compile_status == GL_TRUE);
	}

	static void PrintShaderLog(GLuint shader) 
	{
		GLint log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0) 
		{
			std::unique_ptr<char[]> log_data(new char[log_length]);
			glGetShaderInfoLog(shader, log_length, &log_length, log_data.get());
			std::cerr << "compile log = "
				<< std::string(log_data.get(), log_length) << std::endl;
		}
	}

	static void CheckProgram(GLuint program) 
	{
		GLint link_status;
		glGetProgramiv(program, GL_LINK_STATUS, &link_status);
		if (link_status == GL_FALSE) 
		{
			PrintProgramLog(program);
		}
		assert(link_status == GL_TRUE);
		assert(glGetError() == 0);
	}

	static void PrintProgramLog(GLuint program) 
	{
		GLint log_length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
		if (log_length > 0) 
		{
			std::unique_ptr<char[]> log_data(new char[log_length]);
			glGetProgramInfoLog(program, log_length, &log_length, log_data.get());
			std::cerr << "link log = "
				<< std::string(log_data.get(), log_length) << std::endl;
		}
	}

	GLuint program_;
};

/*
<p>We also need functions to allocate the precomputed textures on GPU:
*/

GLuint NewTexture2d(int width, int height) 
{
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	// 16F precision for the transmittance gives artifacts.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0,
		GL_RGB, GL_FLOAT, NULL);
	return texture;
}

GLuint NewTexture3d(int width, int height, int depth, GLenum format)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	if (format == GL_RGBA)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, width, height, depth, 0,
			format, GL_FLOAT, NULL);
	}
	else
	{
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, width, height, depth, 0,
			format, GL_FLOAT, NULL);
	}
	return texture;
}

/*
<p>and a function to draw a full screen quad in an offscreen framebuffer:
*/

void DrawQuad()
{
	GLfloat vertexPos[] =
	{
		-1.0, -1.0,  // v0
		 1.0, -1.0,  // v1
		-1.0,  1.0,  // v2
		 1.0,  1.0   // v3
	};

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertexPos);
	glEnableVertexAttribArray(0);
	CHECK_GL_ERROR_DEBUG();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	CHECK_GL_ERROR_DEBUG();
	glDisableVertexAttribArray(0);
}

/*
<p>Finally, we need a utility function to compute the value of the conversion
constants *<code>_RADIANCE_TO_LUMINANCE</code>, used above to convert the
spectral results into luminance values. These are the constants k_r, k_g, k_b
described in Section 14.3 of <a href="https://arxiv.org/pdf/1612.04336.pdf">A
Qualitative and Quantitative Evaluation of 8 Clear Sky Models</a>.

<p>Computing their value requires an integral of a function times a CIE color
matching function. Thus, we first need functions to interpolate an arbitrary
function (specified by some samples), and a CIE color matching function
(specified by tabulated values), at an arbitrary wavelength. This is the purpose
of the following two functions:
*/

const int kLambdaMin = 360;
const int kLambdaMax = 830;

double CieColorMatchingFunctionTableValue(double wavelength, int column)
{
	if (wavelength <= kLambdaMin || wavelength >= kLambdaMax)
	{
		return 0.0;
	}
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(std::floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(
	const std::vector<double>& wavelengths,
	const std::vector<double>& wavelength_function,
	double wavelength)
{
	assert(wavelength_function.size() == wavelengths.size());
	if (wavelength < wavelengths[0])
	{
		return wavelength_function[0];
	}
	for (unsigned int i = 0; i < wavelengths.size() - 1; ++i)
	{
		if (wavelength < wavelengths[i + 1])
		{
			double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);

			return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
		}
	}

	return wavelength_function[wavelength_function.size() - 1];
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors(
	const std::vector<double>& wavelengths,
	const std::vector<double>& solar_irradiance,
	double lambda_power, double* k_r, double* k_g, double* k_b) 
{
	*k_r = 0.0;
	*k_g = 0.0;
	*k_b = 0.0;
	double solar_r = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaR);
	double solar_g = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaG);
	double solar_b = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaB);
	int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) 
	{
		double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar =
			xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar =
			xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar =
			xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
		*k_r += r_bar * irradiance / solar_r *
			pow(lambda / SkyModel::kLambdaR, lambda_power);
		*k_g += g_bar * irradiance / solar_g *
			pow(lambda / SkyModel::kLambdaG, lambda_power);
		*k_b += b_bar * irradiance / solar_b *
			pow(lambda / SkyModel::kLambdaB, lambda_power);
	}
	*k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}


/*<h3 id="implementation">Model implementation</h3>

<p>Using the above utility functions and classes, we can now implement the
constructor of the <code>Model</code> class. This constructor generates a piece
of GLSL code that defines an <code>ATMOSPHERE</code> constant containing the
atmosphere parameters (we use constants instead of uniforms to enable constant
folding and propagation optimizations in the GLSL compiler), concatenated with
<a href="functions.glsl.html">functions.glsl</a>, and with
<code>ATMOSPHERE_SHADER</code>, to get the shader exposed by our API in
<code>GetShader</code>. It also allocates the precomputed textures, but does not
initialize them.
*/

double SkyModel::kLambdaR = 680.0;
double SkyModel::kLambdaG = 550.0;
double SkyModel::kLambdaB = 440.0;

SkyModel::SkyModel(
	const std::vector<double>& wavelengths,
	const std::vector<double>& solar_irradiance,
	const double sun_angular_radius,
	double bottom_radius,
	double top_radius,
	double rayleigh_scale_height,
	const std::vector<double>& rayleigh_scattering,
	double mie_scale_height,
	const std::vector<double>& mie_scattering,
	const std::vector<double>& mie_extinction,
	double mie_phase_function_g,
	const std::vector<double>& ground_albedo,
	double max_sun_zenith_angle,
	double length_unit_in_meters,
	bool combine_scattering_textures) {
	auto to_string = [&wavelengths](const std::vector<double>& v, double scale) {
		double r = Interpolate(wavelengths, v, kLambdaR) * scale;
		double g = Interpolate(wavelengths, v, kLambdaG) * scale;
		double b = Interpolate(wavelengths, v, kLambdaB) * scale;
		return "vec3(" + std::to_string(r) + "," + std::to_string(g) + "," +
			std::to_string(b) + ")";
	};
	double sky_k_r, sky_k_g, sky_k_b;
	ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
		-3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
	double sun_k_r, sun_k_g, sun_k_b;
	ComputeSpectralRadianceToLuminanceFactors(wavelengths, solar_irradiance,
		0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);
	glsl_header_ =
		"#version 300 es\n"
		"#define IN(x) const in x\n"
		"#define OUT(x) out x\n"
		"#define TEMPLATE(x)\n"
		"#define TEMPLATE_ARGUMENT(x)\n"
		"#define assert(x)\n"
		"precision mediump float;\n"
		"precision mediump sampler2D;\n"
		"precision mediump sampler3D;\n"
		"const int TRANSMITTANCE_TEXTURE_WIDTH = " +
		std::to_string(TRANSMITTANCE_TEXTURE_WIDTH) + ";\n" +
		"const int TRANSMITTANCE_TEXTURE_HEIGHT = " +
		std::to_string(TRANSMITTANCE_TEXTURE_HEIGHT) + ";\n" +
		"const int SCATTERING_TEXTURE_R_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_R_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_MU_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_MU_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_MU_S_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_MU_S_SIZE) + ";\n" +
		"const int SCATTERING_TEXTURE_NU_SIZE = " +
		std::to_string(SCATTERING_TEXTURE_NU_SIZE) + ";\n" +
		"const int IRRADIANCE_TEXTURE_WIDTH = " +
		std::to_string(IRRADIANCE_TEXTURE_WIDTH) + ";\n" +
		"const int IRRADIANCE_TEXTURE_HEIGHT = " +
		std::to_string(IRRADIANCE_TEXTURE_HEIGHT) + ";\n" +
		(combine_scattering_textures ?
		"#define COMBINED_SCATTERING_TEXTURES\n" : "") +
		getStringFromFile("core/definitions.c") +
		"const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(\n" +
		to_string(solar_irradiance, 1.0) + ",\n" +
		std::to_string(sun_angular_radius) + ",\n" +
		std::to_string(bottom_radius / length_unit_in_meters) + ",\n" +
		std::to_string(top_radius / length_unit_in_meters) + ",\n" +
		std::to_string(
		rayleigh_scale_height / length_unit_in_meters) + ",\n" +
		to_string(rayleigh_scattering, length_unit_in_meters) + ",\n" +
		std::to_string(mie_scale_height / length_unit_in_meters) + ",\n" +
		to_string(mie_scattering, length_unit_in_meters) + ",\n" +
		to_string(mie_extinction, length_unit_in_meters) + ",\n" +
		std::to_string(mie_phase_function_g) + ",\n" +
		to_string(ground_albedo, 1.0) + ",\n" +
		std::to_string(cos(max_sun_zenith_angle)) + ");\n" +
		"const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
		std::to_string(sky_k_r) + "," +
		std::to_string(sky_k_g) + "," +
		std::to_string(sky_k_b) + ");\n" +
		"const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" +
		std::to_string(sun_k_r) + "," +
		std::to_string(sun_k_g) + "," +
		std::to_string(sun_k_b) + ");\n" +
		getStringFromFile("core/functions.c");
	transmittance_texture_ = NewTexture2d(
		TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	scattering_texture_ = NewTexture3d(
		SCATTERING_TEXTURE_WIDTH,
		SCATTERING_TEXTURE_HEIGHT,
		SCATTERING_TEXTURE_DEPTH,
		combine_scattering_textures ? GL_RGBA : GL_RGB);
	if (combine_scattering_textures) {
		optional_single_mie_scattering_texture_ = 0;
	}
	else {
		optional_single_mie_scattering_texture_ = NewTexture3d(
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			GL_RGB);
	}
	irradiance_texture_ = NewTexture2d(
		IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

	atmosphere_shader_str_ = glsl_header_ + kAtmosphereShader;
	//const char* source = atmosphere_shader_str_.c_str();
	//atmosphere_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(atmosphere_shader_, 1, &source, NULL);
	//glCompileShader(atmosphere_shader_);
}

/*
<p>The destructor is trivial:
*/

SkyModel::~SkyModel()
{
	glDeleteTextures(1, &transmittance_texture_);
	glDeleteTextures(1, &scattering_texture_);
	if (optional_single_mie_scattering_texture_ != 0)
	{
		glDeleteTextures(1, &optional_single_mie_scattering_texture_);
	}
	glDeleteTextures(1, &irradiance_texture_);
	glDeleteShader(atmosphere_shader_);
}

std::string SkyModel::getStringFromFile(const char* filename)
{
	std::ifstream ifile(filename);
	//���ļ����뵽ostringstream����buf��
	std::ostringstream buf;
	char ch;
	while (buf&&ifile.get(ch))
	{
		buf.put(ch);
	}
	//������������buf�������ַ���
	return buf.str();
}

/*
<p>The most complex method is the following, which precomputes the atmosphere
textures. This method first allocates the temporary resources it needs, then
performs the precomputations, and finally destroys the temporary resources.
Each phase is explained by the inline comments below.
*/

void SkyModel::Init(unsigned int num_scattering_orders) {
	// The precomputations require temporary textures, in particular to store the
	// contribution of one scattering order, which is needed to compute the next
	// order of scattering (the final precomputed textures store the sum of all
	// the scattering orders). We allocate them here, and destroy them at the end
	// of this method.
	GLuint delta_irradiance_texture = NewTexture2d(
		IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
	GLuint delta_rayleigh_scattering_texture = NewTexture3d(
		SCATTERING_TEXTURE_WIDTH,
		SCATTERING_TEXTURE_HEIGHT,
		SCATTERING_TEXTURE_DEPTH,
		GL_RGB);
	GLuint delta_mie_scattering_texture;
	if (optional_single_mie_scattering_texture_ == 0) {
		delta_mie_scattering_texture = NewTexture3d(
			SCATTERING_TEXTURE_WIDTH,
			SCATTERING_TEXTURE_HEIGHT,
			SCATTERING_TEXTURE_DEPTH,
			GL_RGB);
	}
	else {
		delta_mie_scattering_texture = optional_single_mie_scattering_texture_;
	}
	GLuint delta_scattering_density_texture = NewTexture3d(
		SCATTERING_TEXTURE_WIDTH,
		SCATTERING_TEXTURE_HEIGHT,
		SCATTERING_TEXTURE_DEPTH,
		GL_RGB);
	GLuint delta_multiple_scattering_texture = delta_rayleigh_scattering_texture;

	// The precomputations also require a temporary framebuffer object, created
	// here (and destroyed at the end of this method).
	GLuint fbo, depth_render_buffer;
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &depth_render_buffer);

	// bind renderbuffer and create a 16-bit depth buffer
	// width and height of renderbuffer = width and height of
	// the texture
	//glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	const GLuint kDrawBuffers[3] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};

	glDrawBuffers(1, kDrawBuffers);

	// Finally, the precomputations also require specific GLSL programs, for each
	// precomputation step. We create and compile them here (they are
	// automatically destroyed when this method returns, via the Program
	// destructor).
	Program compute_transmittance(kVertexShader, glsl_header_ + kComputeTransmittanceShader);
	Program compute_direct_irradiance(kVertexShader, glsl_header_ + kComputeDirectIrradianceShader);
	Program compute_single_scattering(kVertexShader, glsl_header_ + kComputeSingleScatteringShader);
	Program compute_scattering_density(kVertexShader, glsl_header_ + kComputeScatteringDensityShader);
	Program compute_indirect_irradiance(kVertexShader, glsl_header_ + kComputeIndirectIrradianceShader);
	Program compute_multiple_scattering(kVertexShader, glsl_header_ + kComputeMultipleScatteringShader);

	CHECK_GL_ERROR_DEBUG();
	// Compute the transmittance, and store it in transmittance_texture_.
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, transmittance_texture_, 0);
	// specify depth_renderbufer as depth attachment
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	// check for framebuffer complete
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer object is not complete!\n");
	}

	glViewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	compute_transmittance.Use();
	CHECK_GL_ERROR_DEBUG();
	DrawQuad();

	CHECK_GL_ERROR_DEBUG();

	// Compute the direct irradiance, store it in delta_irradiance_texture, and
	// initialize irradiance_texture_ with zeros (we don't want the direct
	// irradiance in irradiance_texture_, but only the irradiance from the sky).
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, delta_irradiance_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, irradiance_texture_, 0);
	glDrawBuffers(2, kDrawBuffers);
	glViewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
	compute_direct_irradiance.Use();
	compute_direct_irradiance.BindTexture2d(
		"transmittance_texture", transmittance_texture_, 0);
	DrawQuad();

	CHECK_GL_ERROR_DEBUG();

	// Compute the rayleigh and mie single scattering, and store them in
	// delta_rayleigh_scattering_texture and delta_mie_scattering_texture, as well
	// as in scattering_texture.
	glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
	compute_single_scattering.Use();
	compute_single_scattering.BindTexture2d(
		"transmittance_texture", transmittance_texture_, 0);
	glDrawBuffers(3, kDrawBuffers);
	for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			delta_rayleigh_scattering_texture, 0, layer);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			delta_mie_scattering_texture, 0, layer);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
			scattering_texture_, 0, layer);
		compute_single_scattering.BindFloat("layer", layer);
		DrawQuad();
	}

	CHECK_GL_ERROR_DEBUG();

	// Compute the 2nd, 3rd and 4th order of scattering, in sequence.
	for (unsigned int scattering_order = 2;
		scattering_order <= num_scattering_orders;
		++scattering_order) {
		// Compute the scattering density, and store it in
		// delta_scattering_density_texture.
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, 0, 0);
		glDrawBuffers(1, kDrawBuffers);
		glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
		compute_scattering_density.Use();
		compute_scattering_density.BindTexture2d(
			"transmittance_texture", transmittance_texture_, 0);
		compute_scattering_density.BindTexture3d(
			"single_rayleigh_scattering_texture",
			delta_rayleigh_scattering_texture,
			1);
		compute_scattering_density.BindTexture3d(
			"single_mie_scattering_texture", delta_mie_scattering_texture, 2);
		compute_scattering_density.BindTexture3d(
			"multiple_scattering_texture", delta_multiple_scattering_texture, 3);
		compute_scattering_density.BindTexture2d(
			"irradiance_texture", delta_irradiance_texture, 4);
		compute_scattering_density.BindInt("scattering_order", scattering_order);
		for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				delta_scattering_density_texture, 0, layer);
			compute_scattering_density.BindFloat("layer", layer);
			DrawQuad();
		}

		// Compute the indirect irradiance, store it in delta_irradiance_texture and
		// accumulate it in irradiance_texture_.
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, delta_irradiance_texture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			GL_TEXTURE_2D, irradiance_texture_, 0);
		glDrawBuffers(2, kDrawBuffers);
		glViewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
		compute_indirect_irradiance.Use();
		compute_indirect_irradiance.BindTexture3d(
			"single_rayleigh_scattering_texture",
			delta_rayleigh_scattering_texture,
			0);
		compute_indirect_irradiance.BindTexture3d(
			"single_mie_scattering_texture", delta_mie_scattering_texture, 1);
		compute_indirect_irradiance.BindTexture3d(
			"multiple_scattering_texture", delta_multiple_scattering_texture, 2);
		compute_indirect_irradiance.BindInt("scattering_order", scattering_order);
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
		DrawQuad();
		glDisable(GL_BLEND);

		// Compute the multiple scattering, store it in
		// delta_multiple_scattering_texture, and accumulate it in
		// scattering_texture_.
		glViewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
		compute_multiple_scattering.Use();
		compute_multiple_scattering.BindTexture2d(
			"transmittance_texture", transmittance_texture_, 0);
		compute_multiple_scattering.BindTexture3d(
			"scattering_density_texture", delta_scattering_density_texture, 1);
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
		glDrawBuffers(2, kDrawBuffers);
		for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				delta_multiple_scattering_texture, 0, layer);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				scattering_texture_, 0, layer);
			compute_multiple_scattering.BindFloat("layer", layer);
			DrawQuad();
		}
		glDisable(GL_BLEND);
	}

	CHECK_GL_ERROR_DEBUG();

	// Delete the temporary resources allocated at the begining of this method.
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &delta_scattering_density_texture);
	if (optional_single_mie_scattering_texture_ == 0) {
		glDeleteTextures(1, &delta_mie_scattering_texture);
	}
	glDeleteTextures(1, &delta_rayleigh_scattering_texture);
	glDeleteTextures(1, &delta_irradiance_texture);

	CHECK_GL_ERROR_DEBUG();
}

/*
<p>The <code>SetProgramUniforms</code> method is straightforward: it simply
binds the precomputed textures to the specified texture units, and then sets
the corresponding uniforms in the user provided program to the index of these
texture units.
*/

void SkyModel::SetProgramUniforms(unsigned int program,
	unsigned int transmittance_texture_unit,
	unsigned int scattering_texture_unit,
	unsigned int irradiance_texture_unit,
	unsigned int single_mie_scattering_texture_unit) const {
	glActiveTexture(GL_TEXTURE0 + transmittance_texture_unit);
	glBindTexture(GL_TEXTURE_2D, transmittance_texture_);
	glUniform1i(glGetUniformLocation(program, "transmittance_texture"),
		transmittance_texture_unit);

	glActiveTexture(GL_TEXTURE0 + scattering_texture_unit);
	glBindTexture(GL_TEXTURE_3D, scattering_texture_);
	glUniform1i(glGetUniformLocation(program, "scattering_texture"),
		scattering_texture_unit);

	glActiveTexture(GL_TEXTURE0 + irradiance_texture_unit);
	glBindTexture(GL_TEXTURE_2D, irradiance_texture_);
	glUniform1i(glGetUniformLocation(program, "irradiance_texture"),
		irradiance_texture_unit);

	if (optional_single_mie_scattering_texture_ != 0) {
		glActiveTexture(GL_TEXTURE0 + single_mie_scattering_texture_unit);
		glBindTexture(GL_TEXTURE_3D, optional_single_mie_scattering_texture_);
		glUniform1i(glGetUniformLocation(program, "single_mie_scattering_texture"),
			single_mie_scattering_texture_unit);
	}
}

/*
<p>Finally, the utility method <code>ConvertSpectrumToLinearSrgb</code> is
implemented with a simple numerical integration of the given function, times
the CIE color matching funtions (with an integration step of 1nm), followed by
a matrix multiplication:
*/

void SkyModel::ConvertSpectrumToLinearSrgb(
	const std::vector<double>& wavelengths,
	const std::vector<double>& spectrum,
	double* r, double* g, double* b)
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	const int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double value = Interpolate(wavelengths, spectrum, lambda);
		x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
		y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
		z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
	}
	*r = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
	*g = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
	*b = MAX_LUMINOUS_EFFICACY *
		(XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}

