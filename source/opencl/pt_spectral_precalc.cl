/* (The following OpenCL code is based on the work:)
 *
 *             Colour Rendering of Spectra
 *                   by John Walker
 *               http://www.fourmilab.ch/
 *
 * Last updated: March 9, 2003
 * This program is in the public domain.
 *
 * For complete information about the techniques employed in
 * this program, see the World-Wide Web document:
 *     http://www.fourmilab.ch/documents/specrend/
 *
 * The xyz_to_rgb() function, which was wrong in the original
 * version of this program, was corrected by:
 *
 *    Andrew J. S. Hamilton 21 May 1999
 *    Andrew.Hamilton@Colorado.EDU
 *    http://casa.colorado.edu/~ajsh/
 *
 * who also added the gamma correction facilities and
 * modified constrain_rgb() to work by desaturating the
 * colour by adding white.
 *
 * A program which uses these functions to plot CIE
 * "tongue" diagrams called "ppmcie" is included in
 * the Netpbm graphics toolkit: http://netpbm.sourceforge.net/
 * (The program was called cietoppm in earlier versions of Netpbm.)
 */


/* CIE colour matching functions xBar, yBar, and zBar for
 * wavelengths from 380 through 780 nanometers, every 5
 * nanometers. For a wavelength lambda in this range:
 *     cie_colour_match[(lambda - 380) / 5][0] = xBar
 *     cie_colour_match[(lambda - 380) / 5][1] = yBar
 *     cie_colour_match[(lambda - 380) / 5][2] = zBar
 */
constant float cie_colour_match[90][3] = {
	{ 2.689900e-003f, 2.000000e-004f, 1.226000e-002f },
	{ 5.310500e-003f, 3.955600e-004f, 2.422200e-002f },
	{ 1.078100e-002f, 8.000000e-004f, 4.925000e-002f },
	{ 2.079200e-002f, 1.545700e-003f, 9.513500e-002f },
	{ 3.798100e-002f, 2.800000e-003f, 1.740900e-001f },
	{ 6.315700e-002f, 4.656200e-003f, 2.901300e-001f },
	{ 9.994100e-002f, 7.400000e-003f, 4.605300e-001f },
	{ 1.582400e-001f, 1.177900e-002f, 7.316600e-001f },
	{ 2.294800e-001f, 1.750000e-002f, 1.065800e+000f },
	{ 2.810800e-001f, 2.267800e-002f, 1.314600e+000f },
	{ 3.109500e-001f, 2.730000e-002f, 1.467200e+000f },
	{ 3.307200e-001f, 3.258400e-002f, 1.579600e+000f },
	{ 3.333600e-001f, 3.790000e-002f, 1.616600e+000f },
	{ 3.167200e-001f, 4.239100e-002f, 1.568200e+000f },
	{ 2.888200e-001f, 4.680000e-002f, 1.471700e+000f },
	{ 2.596900e-001f, 5.212200e-002f, 1.374000e+000f },
	{ 2.327600e-001f, 6.000000e-002f, 1.291700e+000f },
	{ 2.099900e-001f, 7.294200e-002f, 1.235600e+000f },
	{ 1.747600e-001f, 9.098000e-002f, 1.113800e+000f },
	{ 1.328700e-001f, 1.128400e-001f, 9.422000e-001f },
	{ 9.194400e-002f, 1.390200e-001f, 7.559600e-001f },
	{ 5.698500e-002f, 1.698700e-001f, 5.864000e-001f },
	{ 3.173100e-002f, 2.080200e-001f, 4.466900e-001f },
	{ 1.461300e-002f, 2.580800e-001f, 3.411600e-001f },
	{ 4.849100e-003f, 3.230000e-001f, 2.643700e-001f },
	{ 2.321500e-003f, 4.054000e-001f, 2.059400e-001f },
	{ 9.289900e-003f, 5.030000e-001f, 1.544500e-001f },
	{ 2.927800e-002f, 6.081100e-001f, 1.091800e-001f },
	{ 6.379100e-002f, 7.100000e-001f, 7.658500e-002f },
	{ 1.108100e-001f, 7.951000e-001f, 5.622700e-002f },
	{ 1.669200e-001f, 8.620000e-001f, 4.136600e-002f },
	{ 2.276800e-001f, 9.150500e-001f, 2.935300e-002f },
	{ 2.926900e-001f, 9.540000e-001f, 2.004200e-002f },
	{ 3.622500e-001f, 9.800400e-001f, 1.331200e-002f },
	{ 4.363500e-001f, 9.949500e-001f, 8.782300e-003f },
	{ 5.151300e-001f, 1.000100e+000f, 5.857300e-003f },
	{ 5.974800e-001f, 9.950000e-001f, 4.049300e-003f },
	{ 6.812100e-001f, 9.787500e-001f, 2.921700e-003f },
	{ 7.642500e-001f, 9.520000e-001f, 2.277100e-003f },
	{ 8.439400e-001f, 9.155800e-001f, 1.970600e-003f },
	{ 9.163500e-001f, 8.700000e-001f, 1.806600e-003f },
	{ 9.770300e-001f, 8.162300e-001f, 1.544900e-003f },
	{ 1.023000e+000f, 7.570000e-001f, 1.234800e-003f },
	{ 1.051300e+000f, 6.948300e-001f, 1.117700e-003f },
	{ 1.055000e+000f, 6.310000e-001f, 9.056400e-004f },
	{ 1.036200e+000f, 5.665400e-001f, 6.946700e-004f },
	{ 9.923900e-001f, 5.030000e-001f, 4.288500e-004f },
	{ 9.286100e-001f, 4.417200e-001f, 3.181700e-004f },
	{ 8.434600e-001f, 3.810000e-001f, 2.559800e-004f },
	{ 7.398300e-001f, 3.205200e-001f, 1.567900e-004f },
	{ 6.328900e-001f, 2.650000e-001f, 9.769400e-005f },
	{ 5.335100e-001f, 2.170200e-001f, 6.894400e-005f },
	{ 4.406200e-001f, 1.750000e-001f, 5.116500e-005f },
	{ 3.545300e-001f, 1.381200e-001f, 3.601600e-005f },
	{ 2.786200e-001f, 1.070000e-001f, 2.423800e-005f },
	{ 2.148500e-001f, 8.165200e-002f, 1.691500e-005f },
	{ 1.616100e-001f, 6.100000e-002f, 1.190600e-005f },
	{ 1.182000e-001f, 4.432700e-002f, 8.148900e-006f },
	{ 8.575300e-002f, 3.200000e-002f, 5.600600e-006f },
	{ 6.307700e-002f, 2.345400e-002f, 3.954400e-006f },
	{ 4.583400e-002f, 1.700000e-002f, 2.791200e-006f },
	{ 3.205700e-002f, 1.187200e-002f, 1.917600e-006f },
	{ 2.218700e-002f, 8.210000e-003f, 1.313500e-006f },
	{ 1.561200e-002f, 5.772300e-003f, 9.151900e-007f },
	{ 1.109800e-002f, 4.102000e-003f, 6.476700e-007f },
	{ 7.923300e-003f, 2.929100e-003f, 4.635200e-007f },
	{ 5.653100e-003f, 2.091000e-003f, 3.330400e-007f },
	{ 4.003900e-003f, 1.482200e-003f, 2.382300e-007f },
	{ 2.825300e-003f, 1.047000e-003f, 1.702600e-007f },
	{ 1.994700e-003f, 7.401500e-004f, 1.220700e-007f },
	{ 1.399400e-003f, 5.200000e-004f, 8.710700e-008f },
	{ 9.698000e-004f, 3.609300e-004f, 6.145500e-008f },
	{ 6.684700e-004f, 2.492000e-004f, 4.316200e-008f },
	{ 4.614100e-004f, 1.723100e-004f, 3.037900e-008f },
	{ 3.207300e-004f, 1.200000e-004f, 2.155400e-008f },
	{ 2.257300e-004f, 8.462000e-005f, 1.549300e-008f },
	{ 1.597300e-004f, 6.000000e-005f, 1.120400e-008f },
	{ 1.127500e-004f, 4.244600e-005f, 8.087300e-009f },
	{ 7.951300e-005f, 3.000000e-005f, 5.834000e-009f },
	{ 5.608700e-005f, 2.121000e-005f, 4.211000e-009f },
	{ 3.954100e-005f, 1.498900e-005f, 3.038300e-009f },
	{ 2.785200e-005f, 1.058400e-005f, 2.190700e-009f },
	{ 1.959700e-005f, 7.465600e-006f, 1.577800e-009f },
	{ 1.377000e-005f, 5.259200e-006f, 1.134800e-009f },
	{ 9.670000e-006f, 3.702800e-006f, 8.156500e-010f },
	{ 6.791800e-006f, 2.607600e-006f, 5.862600e-010f },
	{ 4.770600e-006f, 1.836500e-006f, 4.213800e-010f },
	{ 3.355000e-006f, 1.295000e-006f, 3.031900e-010f },
	{ 2.353400e-006f, 9.109200e-007f, 2.175300e-010f },
	{ 1.637700e-006f, 6.356400e-007f, 1.547600e-010f }
};


/**
 * If the requested RGB shade contains a negative weight for
 * one of the primaries, it lies outside the colour gamut
 * accessible from the given triple of primaries. Desaturate
 * it by adding white, equal quantities of R, G, and B, enough
 * to make RGB all positive. The function returns 1 if the
 * components were modified, zero otherwise.
 *
 * @param  {float4*} rgb
 * @return {bool}        True, if rgb has been modified, false otherwise.
 */
bool constrain_rgb( float4* rgb ) {
	// Amount of white needed is w = -min(0, r, g, b)
	float w = -fmin( fmin( 0.0f, rgb->x ), fmin( rgb->y, rgb->z ) );

	// Add just enough white to make r, g, b all positive.
	if( w > 0.0f ) {
		rgb->x += w;
		rgb->y += w;
		rgb->z += w;

		return true;
	}

	return false;
}


/**
 * Scale the RGB values so the greatest value is 1.0,
 * unless all values are 0.0.
 * @param {float4*} rgb RGB.
 */
void scaleRGB( float4* rgb ) {
	float greatest = fmax( rgb->x, fmax( rgb->y, rgb->z ) );

	if( greatest > 0.0f ) {
		*rgb /= greatest;
	}
}


/**
 * Given an additive tricolour system CS, defined by the CIE x
 * and y chromaticities of its three primaries (z is derived
 * trivially as 1-(x+y)), and a desired chromaticity (XC, YC,
 * ZC) in CIE space, determine the contribution of each
 * primary in a linear combination which sums to the desired
 * chromaticity. If the requested chromaticity falls outside
 * the Maxwell triangle (colour gamut) formed by the three
 * primaries, one of the r, g, or b weights will be negative.
 *
 * Caller can use constrain_rgb() to desaturate an
 * outside-gamut colour to the closest representation within
 * the available gamut and/or norm_rgb to normalise the RGB
 * components so the largest nonzero component has value 1.
 *
 * @param {const float4} xyz
 * @param {float4*}      rgb
 */
void xyz_to_rgb( const float4 xyz, float4* rgb ) {
	// Using a pre-calculated matrix
	// For the calculations see "opencl/pt_spectral.cl".

	#if SPECTRAL_COLORSYSTEM == 0

		rgb->x =  6.040009f * xyz.x + -1.683788f * xyz.y + -0.911408f * xyz.z;
		rgb->y = -3.113923f * xyz.x +  6.322208f * xyz.y + -0.089522f * xyz.z;
		rgb->z =  0.184473f * xyz.x + -0.374537f * xyz.y +  2.839774f * xyz.z;

	#elif SPECTRAL_COLORSYSTEM == 1

		rgb->x =  9.313725f * xyz.x + -4.236410f * xyz.y + -1.446676f * xyz.z;
		rgb->y = -2.944388f * xyz.x +  5.698851f * xyz.y +  0.126237f * xyz.z;
		rgb->z =  0.206346f * xyz.x + -0.695713f * xyz.y +  3.250795f * xyz.z;

	#elif SPECTRAL_COLORSYSTEM == 2

		rgb->x = 10.660419f * xyz.x + -5.290040f * xyz.y + -1.654274f * xyz.z;
		rgb->y = -3.247467f * xyz.x +  6.007939f * xyz.y +  0.106841f * xyz.z;
		rgb->z =  0.171210f * xyz.x + -0.598937f * xyz.y +  3.192554f * xyz.z;

	#elif SPECTRAL_COLORSYSTEM == 3

		rgb->x =  6.205850f * xyz.x + -1.717461f * xyz.y + -1.047886f * xyz.z;
		rgb->y = -2.715540f * xyz.x +  5.513369f * xyz.y +  0.096872f * xyz.z;
		rgb->z =  0.193850f * xyz.x + -0.393574f * xyz.y +  2.984110f * xyz.z;

	#elif SPECTRAL_COLORSYSTEM == 4

		rgb->x =  6.863516f * xyz.x + -2.500103f * xyz.y + -1.363412f * xyz.z;
		rgb->y = -1.534954f * xyz.x +  4.268275f * xyz.y +  0.266679f * xyz.z;
		rgb->z =  0.017161f * xyz.x + -0.047721f * xyz.y +  3.030559f * xyz.z;

	#elif SPECTRAL_COLORSYSTEM == 5

		rgb->x =  9.854084f * xyz.x + -4.674373f * xyz.y + -1.516013f * xyz.z;
		rgb->y = -2.944388f * xyz.x +  5.698851f * xyz.y +  0.126237f * xyz.z;
		rgb->z =  0.169153f * xyz.x + -0.620228f * xyz.y +  3.213911f * xyz.z;

	#endif
}


/**
 * Test whether a requested colour is within the gamut
 * achievable with the primaries of the current colour
 * system. This amounts simply to testing whether all the
 * primary weights are non-negative.
 *
 * @param  {float4} rgb
 * @return {bool}       True, if each rgb component is inside gamut, false otherwise.
 */
inline bool inside_gamut( float4 rgb ) {
	return ( rgb.x >= 0.0f && rgb.y >= 0.0f && rgb.z >= 0.0f );
}


/**
 * Transform linear RGB values to nonlinear RGB values. Rec.
 * 709 is ITU-R Recommendation BT. 709 (1990) "Basic
 * Parameter Values for the HDTV Standard for the Studio and
 * for International Programme Exchange", formerly CCIR Rec.
 * 709. For details see:
 *     http://www.poynton.com/ColorFAQ.html
 *     http://www.poynton.com/GammaFAQ.html
 *
 * @param {float*} c
 */
void gamma_correct( float* c ) {
	// Rec. 709 gamma correction.
	const float cc = 0.018f;

	if( *c < cc ) {
		*c *= native_divide( 1.099f * pow( cc, 0.45f ) - 0.099f, cc );
	}
	else {
		*c = 1.099f * pow( *c, 0.45f ) - 0.099f;
	}
}


/**
 * Gamma correct each rgb component.
 * @param {float*} r
 * @param {float*} g
 * @param {float*} b
 */
void gamma_correct_rgb( float4* rgb ) {
	float r = rgb->x;
	float g = rgb->y;
	float b = rgb->z;
	gamma_correct( &r );
	gamma_correct( &g );
	gamma_correct( &b );
	rgb->x = r;
	rgb->y = g;
	rgb->z = b;
}


/**
 * Calculate the CIE X, Y, and Z coordinates corresponding to
 * a light source with spectral distribution given by the
 * function SPEC_INTENS, which is called with a series of
 * wavelengths between 380 and 780 nm (the argument is
 * expressed in meters), which returns emittance at that
 * wavelength in arbitrary units. The chromaticity
 * coordinates of the spectrum are returned in the x, y, and z
 * arguments which respect the identity:
 *         x + y + z = 1.
 *
 * @param {const float*} spd Spectral power distribution.
 * @param {float4*}      x
 */
float spectrum_to_xyz( const float spd[SPEC], float4* xyz ) {
	float me;
	float X = 0.0f,
	      Y = 0.0f,
	      Z = 0.0f;
	float4 maxPossible = (float4)( 0.0f );
	const uint stepI = 80 / SPEC;

	for( int i = 0, j = 0; j < SPEC; i += stepI, j++ ) {
		me = spd[j];
		X += me * cie_colour_match[i][0];
		Y += me * cie_colour_match[i][1];
		Z += me * cie_colour_match[i][2];

		maxPossible.x += cie_colour_match[i][0];
		maxPossible.y += cie_colour_match[i][1];
		maxPossible.z += cie_colour_match[i][2];
	}

	const float XYZmax = maxPossible.x + maxPossible.y + maxPossible.z;
	const float XYZsum = X + Y + Z;
	const float intensity = 1.0f - native_divide( XYZmax - XYZsum, XYZmax );

	xyz->x = native_divide( X, XYZsum );
	xyz->y = native_divide( Y, XYZsum );
	xyz->z = native_divide( Z, XYZsum );

	return intensity;
}


/**
 * Scale the SPD values between 0.0 and 1.0, but only
 * if a value is greater than 1.0.
 * @param {float*} spd The SPD to scale.
 */
void scaleSPD( float spd[SPEC] ) {
	float maxVal = 0.0f;

	for( uint i = 0; i < SPEC; i++ ) {
		maxVal = fmax( maxVal, spd[i] );
	}

	if( maxVal <= 1.0f ) {
		return;
	}

	for( uint j = 0; j < SPEC; j++ ) {
		spd[j] = native_divide( spd[j], maxVal );
	}
}


/**
 * Write the final color to the output image.
 * @param {const int2}           pos         Pixel coordinate in the image to read from/write to.
 * @param {read_only image2d_t}  imageIn     The previously generated image.
 * @param {write_only image2d_t} imageOut    Output.
 * @param {const float}          pixelWeight Mixing weight of the new color with the old one.
 * @param {float[40]}            spdLight    Spectral power distribution reaching this pixel.
 * @param {float}                focus       Value <t> of the ray.
 */
void setColors(
	read_only image2d_t imageIn, write_only image2d_t imageOut,
	const float pixelWeight, float spdLight[40], float focus
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );
	const float4 accumulatedColor = spectrumToRGB( spdLight );

	float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		imagePixel, pixelWeight
	);
	color.w = focus;

	write_imagef( imageOut, pos, color );
}


/**
 * Convert a given spectrum into RGB.
 * @param  {float*} spd Spectral power distribution.
 * @return {float4}     RGB.
 */
float4 spectrumToRGB( float spd[SPEC] ) {
	float4 rgb = (float4)( 0.0f );
	float4 xyz = (float4)( 0.0f );

	scaleSPD( spd );
	const float intensity = spectrum_to_xyz( spd, &xyz );
	xyz_to_rgb( xyz, &rgb );
	rgb *= intensity;
	gamma_correct_rgb( &rgb );
	constrain_rgb( &rgb );

	return rgb;
}
