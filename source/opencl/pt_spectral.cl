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


/* A colour system is defined by the CIE x and y coordinates of
 * its three primary illuminants and the x and y coordinates of
 * the white point.
 */
typedef struct {
	float xRed, yRed,     /* Red x, y */
	      xGreen, yGreen, /* Green x, y */
	      xBlue, yBlue,   /* Blue x, y */
	      xWhite, yWhite, /* White point x, y */
	      gamma;          /* Gamma correction for system */
} colorSystem;


// White point chromaticities.
#define IlluminantC   0.3101f, 0.3162f         // For NTSC television
#define IlluminantD65 0.3127f, 0.3291f         // For EBU and SMPTE
#define IlluminantE   0.33333333f, 0.33333333f // CIE equal-energy illuminant


/* Gamma of nonlinear correction.
 * See Charles Poynton's ColorFAQ Item 45 and GammaFAQ Item 6 at:
 *     http://www.poynton.com/ColorFAQ.html
 *     http://www.poynton.com/GammaFAQ.html
 */
#define GAMMA_REC709 0.0f // Rec. 709

	                         /* xRed     yRed     xGreen   yGreen   xBlue    yBlue    White point    Gamma */
#if SPECTRAL_COLORSYSTEM == 0 // NTSC

	constant colorSystem CS = { 0.67f,   0.33f,   0.21f,   0.71f,   0.14f,   0.08f,   IlluminantC,   GAMMA_REC709 };

#elif SPECTRAL_COLORSYSTEM == 1 // EBU

	constant colorSystem CS = { 0.64f,   0.33f,   0.29f,   0.60f,   0.15f,   0.06f,   IlluminantD65, GAMMA_REC709 };

#elif SPECTRAL_COLORSYSTEM == 2 // SMPTE

	constant colorSystem CS = { 0.630f,  0.340f,  0.310f,  0.595f,  0.155f,  0.070f,  IlluminantD65, GAMMA_REC709 };

#elif SPECTRAL_COLORSYSTEM == 3 // HDTV

	constant colorSystem CS = { 0.670f,  0.330f,  0.210f,  0.710f,  0.150f,  0.060f,  IlluminantD65, GAMMA_REC709 };

#elif SPECTRAL_COLORSYSTEM == 4 // CIE

	constant colorSystem CS = { 0.7355f, 0.2645f, 0.2658f, 0.7243f, 0.1669f, 0.0085f, IlluminantE,   GAMMA_REC709 };

#elif SPECTRAL_COLORSYSTEM == 5 // Rec709

	constant colorSystem CS = { 0.64f,   0.33f,   0.30f,   0.60f,   0.15f,   0.06f,   IlluminantD65, GAMMA_REC709 };

#endif


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
 * @param {const colorSystem} cs
 * @param {float}             xc
 * @param {float}             yc
 * @param {float}             zc
 * @param {float*}            r
 * @param {float*}            g
 * @param {float*}            b
 */
void xyz_to_rgb( const colorSystem cs, float4 xyz, float4* rgb ) {
	float xr = cs.xRed;
	float yr = cs.yRed;
	float zr = 1.0f - ( xr + yr );

	float xg = cs.xGreen;
	float yg = cs.yGreen;
	float zg = 1.0f - ( xg + yg );

	float xb = cs.xBlue;
	float yb = cs.yBlue;
	float zb = 1.0f - ( xb + yb );

	float xw = cs.xWhite;
	float yw = cs.yWhite;
	float zw = 1.0f - xw - yw;


	// http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_RGB.html

	float Xr = xr / yr;
	float Xg = xg / yg;
	float Xb = xb / yb;

	float Zr = zr / yr;
	float Zg = zg / yg;
	float Zb = zb / yb;


	float detN1 = Xr * ( Zb - Zg );
	float detN2 = Xg * ( Zb - Zr );
	float detN3 = Xb * ( Zg - Zr );
	float detNrecip = 1.0f / ( detN1 - detN2 + detN3 );

	float n00 = detNrecip * ( Zb - Zg );
	float n01 = detNrecip * ( Xb * Zg - Xg * Zb );
	float n02 = detNrecip * ( Xg - Xb );

	float n10 = detNrecip * ( Zr - Zb );
	float n11 = detNrecip * ( Xr * Zb - Xb * Zr );
	float n12 = detNrecip * ( Xb - Xr );

	float n20 = detNrecip * ( Zg - Zr );
	float n21 = detNrecip * ( Xg * Zr - Xr * Zg );
	float n22 = detNrecip * ( Xr - Xg );


	float sr = n00 * xw + n01 * yw + n02 * zw;
	float sg = n10 * xw + n11 * yw + n12 * zw;
	float sb = n20 * xw + n21 * yw + n22 * zw;


	float m00 = sr * Xr;
	float m01 = sg * Xg;
	float m02 = sb * Xb;

	float m10 = sr;
	float m11 = sg;
	float m12 = sb;

	float m20 = sr * Zr;
	float m21 = sg * Zg;
	float m22 = sb * Zb;


	float sMul = sr * sg * sb;
	float detM1 = sMul * Xr * ( Zb - Zg );
	float detM2 = sMul * Xg * ( Zb - Zr );
	float detM3 = sMul * Xb * ( Zg - Zr );
	float detMrecip = 1.0f / ( detM1 - detM2 + detM3 );

	float m00inv = detMrecip * sg * sb * ( Zb - Zg );
	float m01inv = detMrecip * sg * sb * ( Xb * Zg - Xg * Zb );
	float m02inv = detMrecip * sg * sb * ( Xg - Xb );

	float m10inv = detMrecip * sb * sr * ( Zr - Zb );
	float m11inv = detMrecip * sb * sr * ( Xr * Zb - Xb * Zr );
	float m12inv = detMrecip * sb * sr * ( Xb - Xr );

	float m20inv = detMrecip * sg * sr * ( Zg - Zr );
	float m21inv = detMrecip * sg * sr * ( Xg * Zr - Xr * Zg );
	float m22inv = detMrecip * sg * sr * ( Xr - Xg );


	rgb->x = m00inv * xyz.x + m01inv * xyz.y + m02inv * xyz.z;
	rgb->y = m10inv * xyz.x + m11inv * xyz.y + m12inv * xyz.z;
	rgb->z = m20inv * xyz.x + m21inv * xyz.y + m22inv * xyz.z;
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
 * @param {const colorSystem} cs
 * @param {float*}            c
 */
void gamma_correct( const colorSystem cs, float* c ) {
	float gamma = cs.gamma;

	if( gamma == GAMMA_REC709 ) {
		// Rec. 709 gamma correction.
		float cc = 0.018f;

		if( *c < cc ) {
			*c *= native_divide( 1.099f * pow( cc, 0.45f ) - 0.099f, cc );
		}
		else {
			*c = 1.099f * pow( *c, 0.45f ) - 0.099f;
		}
	}
	else {
		// Nonlinear colour = (Linear colour)^(1/gamma)
		*c = pow( *c, native_recip( gamma ) );
	}
}


/**
 * Gamma correct each rgb component.
 * @param {const colorSystem} cs
 * @param {float*}            r
 * @param {float*}            g
 * @param {float*}            b
 */
void gamma_correct_rgb( const colorSystem cs, float4* rgb ) {
	float r = rgb->x;
	float g = rgb->y;
	float b = rgb->z;
	gamma_correct( cs, &r );
	gamma_correct( cs, &g );
	gamma_correct( cs, &b );
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
 * @param {float*} spd Spectral power distribution.
 * @param {float*} x
 * @param {float*} y
 * @param {float*} z
 */
float spectrum_to_xyz( float spd[SPEC], float4* xyz ) {
	float me;
	float X = 0.0f,
	      Y = 0.0f,
	      Z = 0.0f;
	float lambda = 380.0f;
	float4 maxPossible = (float4)( 0.0f );
	float stepLambda = 400.0f / SPEC;
	int stepI = 80 / SPEC;

	for( int i = 0, j = 0; lambda < 780.0f; i += stepI, j++, lambda += stepLambda ) {
		me = spd[j];
		X += me * cie_colour_match[i][0];
		Y += me * cie_colour_match[i][1];
		Z += me * cie_colour_match[i][2];

		maxPossible.x += cie_colour_match[i][0];
		maxPossible.y += cie_colour_match[i][1];
		maxPossible.z += cie_colour_match[i][2];
	}

	float XYZmax = maxPossible.x + maxPossible.y + maxPossible.z;
	float XYZsum = X + Y + Z;
	float intensity = 1.0f - ( XYZmax - XYZsum ) / XYZmax;

	xyz->x = X / XYZsum;
	xyz->y = Y / XYZsum;
	xyz->z = Z / XYZsum;

	return intensity;
}


/**
 * Convert a given spectrum into RGB.
 * @param  {float*} spd Spectral power distribution.
 * @return {float4}     RGB.
 */
float4 spectrumToRGB( float spd[SPEC] ) {
	float4 rgb = (float4)( 0.0f );
	float4 xyz = (float4)( 0.0f );

	float intensity = spectrum_to_xyz( spd, &xyz );
	xyz_to_rgb( CS, xyz, &rgb );
	rgb *= intensity;
	gamma_correct_rgb( CS, &rgb );
	constrain_rgb( &rgb );

	return rgb;
}
