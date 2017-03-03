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

/*<h2>atmosphere/model.h</h2>

<p>This file defines the API to use our atmosphere model in OpenGL applications.
To use it:
<ul>
<li>create a <code>Model</code> instance with the desired atmosphere
parameters.</li>
<li>call <code>Init</code> to precompute the atmosphere textures,</li>
<li>link <code>GetShader</code> with your shaders that need access to the
atmosphere shading functions.</li>
<li>for each GLSL program linked with <code>GetShader</code>, call
<code>SetProgramUniforms</code> to bind the precomputed textures to this
program (usually at each frame).</li>
<li>delete your <code>Model</code> when you no longer need its shader and
precomputed textures (the destructor deletes these resources).</li>
</ul>

<p>The shader returned by <code>GetShader</code> provides the following
functions (that you need to forward declare in your own shaders to be able to
compile them separately):

<pre class="prettyprint">
// Returns the sky radiance along the segment from 'camera' to the nearest
// atmosphere boundary in direction 'view_ray', as well as the transmittance
// along this segment.
vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, double shadow_length,
vec3 sun_direction, out vec3 transmittance);

// Returns the sky radiance along the segment from 'camera' to 'p', as well as
// the transmittance along this segment.
vec3 GetSkyRadianceToPoint(vec3 camera, vec3 p, double shadow_length,
vec3 sun_direction, out vec3 transmittance);

// Returns the sun and sky irradiance received on a surface patch located at 'p'
// and whose normal vector is 'normal'.
vec3 GetSunAndSkyIrradiance(vec3 p, vec3 normal, vec3 sun_direction,
out vec3 sky_irradiance);

// Returns the sky luminance along the segment from 'camera' to the nearest
// atmosphere boundary in direction 'view_ray', as well as the transmittance
// along this segment.
vec3 GetSkyLuminance(vec3 camera, vec3 view_ray, double shadow_length,
vec3 sun_direction, out vec3 transmittance);

// Returns the sky luminance along the segment from 'camera' to 'p', as well as
// the transmittance along this segment.
vec3 GetSkyLuminanceToPoint(vec3 camera, vec3 p, double shadow_length,
vec3 sun_direction, out vec3 transmittance);

// Returns the sun and sky illuminance received on a surface patch located at
// 'p' and whose normal vector is 'normal'.
vec3 GetSunAndSkyIlluminance(vec3 p, vec3 normal, vec3 sun_direction,
out vec3 sky_illuminance);
</pre>

<p>where
<ul>
<li><code>camera</code> and <code>p</code> must be expressed in a reference
frame where the planet center is at the origin, and measured in the unit passed
to the constructor's <code>length_unit_in_meters</code> argument.
<code>camera</code> can be in space, but <code>p</code> must be inside the
atmosphere,</li>
<li><code>view_ray</code>, <code>sun_direction</code> and <code>normal</code>
are unit direction vectors expressed in the same reference frame (with
<code>sun_direction</code> pointing <i>towards</i> the Sun),</li>
<li><code>shadow_length</code> is the length along the segment which is in
shadow, measured in the unit passed to the constructor's
<code>length_unit_in_meters</code> argument.</li>
</ul>

<p>and where
<ul>
<li>the first 3 functions return spectral radiance and irradiance values
(in $W.m^{-2}.sr^{-1}.nm^{-1}$ and $W.m^{-2}.nm^{-1}$), at the 3 wavelengths
<code>kLambdaR</code>, <code>kLambdaG</code>, <code>kLambdaB</code> (in this
order),</li>
<li>the other functions return luminance and illuminance values (in
$cd.m^{-2}$ and $lx$) in linear <a href="https://en.wikipedia.org/wiki/SRGB">
sRGB</a> space (i.e. before adjustements for gamma correction),</li>
<li>all the functions return the (unitless) transmittance of the atmosphere
along the specified segment at the 3 wavelengths <code>kLambdaR</code>,
<code>kLambdaG</code>, <code>kLambdaB</code> (in this order).</li>
</ul>

<p>The concrete API definition is the following:
*/

#ifndef ATMOSPHERE_MODEL_H_
#define ATMOSPHERE_MODEL_H_

#ifndef GLUT_DISABLE_ATEXIT_HACK  
#define GLUT_DISABLE_ATEXIT_HACK 
#endif  

#include <string>
#include <vector>

class SkyModel {
public:
	SkyModel(
		// The wavelength values, in nanometers, and sorted in increasing order, for
		// which the solar_irradiance, rayleigh_scattering, mie_scattering,
		// mie_extinction and ground_albedo samples are provided. If your shaders
		// use luminance values (as opposed to radiance values, see above), use a
		// large number of wavelengths (e.g. between 15 and 50) to get accurate
		// results (this number of wavelengths has absolutely no impact on the
		// shader performance).
		const std::vector<double>& wavelengths,
		// The solar irradiance at the top of the atmosphere, in W/m^2/nm. This
		// vector must have the same size as the wavelength parameter.
		const std::vector<double>& solar_irradiance,
		// The sun's angular radius, in radians.
		double sun_angular_radius,
		// The distance between the planet center and the bottom of the atmosphere,
		// in m.
		double bottom_radius,
		// The distance between the planet center and the top of the atmosphere,
		// in m.
		double top_radius,
		// The scale height of air molecules, in m, meaning that their density is
		// proportional to exp(-h / rayleigh_scale_height), with h the altitude
		// (with the bottom of the atmosphere at altitude 0).
		double rayleigh_scale_height,
		// The scattering coefficient of air molecules at the bottom of the
		// atmosphere, as a function of wavelength, in m^-1. This vector must have
		// the same size as the wavelength parameter.
		const std::vector<double>& rayleigh_scattering,
		// The scale height of aerosols, in m, meaning that their density is
		// proportional to exp(-h / mie_scale_height), with h the altitude.
		double mie_scale_height,
		// The scattering coefficient of aerosols at the bottom of the atmosphere,
		// as a function of wavelength, in m^-1. This vector must have the same size
		// as the wavelength parameter.
		const std::vector<double>& mie_scattering,
		// The extinction coefficient of aerosols at the bottom of the atmosphere,
		// as a function of wavelength, in m^-1. This vector must have the same size
		// as the wavelength parameter.
		const std::vector<double>& mie_extinction,
		// The asymetry parameter for the Cornette-Shanks phase function for the
		// aerosols.
		double mie_phase_function_g,
		// The average albedo of the ground, as a function of wavelength. This
		// vector must have the same size as the wavelength parameter.
		const std::vector<double>& ground_albedo,
		// The maximum Sun zenith angle for which atmospheric scattering must be
		// precomputed, in radians (for maximum precision, use the smallest Sun
		// zenith angle yielding negligible sky light radiance values. For instance,
		// for the Earth case, 102 degrees is a good choice).
		double max_sun_zenith_angle,
		// The length unit used in your shaders and meshes. This is the length unit
		// which must be used when calling the atmosphere model shader functions.
		double length_unit_in_meters,
		// Whether to pack the (red component of the) single Mie scattering with the
		// Rayleigh and multiple scattering in a single texture, or to store the
		// (3 components of the) single Mie scattering in a separate texture.
		bool combine_scattering_textures);

	~SkyModel();

	std::string getStringFromFile(const char* filename);

	void Init(unsigned int num_scattering_orders = 4);

	unsigned int GetShader() const { return atmosphere_shader_; }

	void SetProgramUniforms(unsigned int program,
		unsigned int transmittance_texture_unit,
		unsigned int scattering_texture_unit,
		unsigned int irradiance_texture_unit,
		unsigned int optional_single_mie_scattering_texture_unit = 0) const;

	// Utility method to convert a function of the wavelength to linear sRGB.
	// 'wavelengths' and 'spectrum' must have the same size. The integral of
	// 'spectrum' times each CIE_2_DEG_COLOR_MATCHING_FUNCTIONS (and times
	// MAX_LUMINOUS_EFFICACY) is computed to get XYZ values, which are then
	// converted to linear sRGB with the XYZ_TO_SRGB matrix.
	static void ConvertSpectrumToLinearSrgb(
		const std::vector<double>& wavelengths,
		const std::vector<double>& spectrum,
		double* r, double* g, double* b);

	static double kLambdaR;
	static double kLambdaG;
	static double kLambdaB;

private:
	std::string glsl_header_;
	unsigned int transmittance_texture_;
	unsigned int scattering_texture_;
	unsigned int optional_single_mie_scattering_texture_;
	unsigned int irradiance_texture_;
	unsigned int atmosphere_shader_;
};

#endif  // ATMOSPHERE_MODEL_H_
