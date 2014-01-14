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
constant float cie_colour_match[80][3] = {
    { 0.0014f, 0.0000f, 0.0065f }, { 0.0022f, 0.0001f, 0.0105f }, { 0.0042f, 0.0001f, 0.0201f },
    { 0.0076f, 0.0002f, 0.0362f }, { 0.0143f, 0.0004f, 0.0679f }, { 0.0232f, 0.0006f, 0.1102f },
    { 0.0435f, 0.0012f, 0.2074f }, { 0.0776f, 0.0022f, 0.3713f }, { 0.1344f, 0.0040f, 0.6456f },
    { 0.2148f, 0.0073f, 1.0391f }, { 0.2839f, 0.0116f, 1.3856f }, { 0.3285f, 0.0168f, 1.6230f },
    { 0.3483f, 0.0230f, 1.7471f }, { 0.3481f, 0.0298f, 1.7826f }, { 0.3362f, 0.0380f, 1.7721f },
    { 0.3187f, 0.0480f, 1.7441f }, { 0.2908f, 0.0600f, 1.6692f }, { 0.2511f, 0.0739f, 1.5281f },
    { 0.1954f, 0.0910f, 1.2876f }, { 0.1421f, 0.1126f, 1.0419f }, { 0.0956f, 0.1390f, 0.8130f },
    { 0.0580f, 0.1693f, 0.6162f }, { 0.0320f, 0.2080f, 0.4652f }, { 0.0147f, 0.2586f, 0.3533f },
    { 0.0049f, 0.3230f, 0.2720f }, { 0.0024f, 0.4073f, 0.2123f }, { 0.0093f, 0.5030f, 0.1582f },
    { 0.0291f, 0.6082f, 0.1117f }, { 0.0633f, 0.7100f, 0.0782f }, { 0.1096f, 0.7932f, 0.0573f },
    { 0.1655f, 0.8620f, 0.0422f }, { 0.2257f, 0.9149f, 0.0298f }, { 0.2904f, 0.9540f, 0.0203f },
    { 0.3597f, 0.9803f, 0.0134f }, { 0.4334f, 0.9950f, 0.0087f }, { 0.5121f, 1.0000f, 0.0057f },
    { 0.5945f, 0.9950f, 0.0039f }, { 0.6784f, 0.9786f, 0.0027f }, { 0.7621f, 0.9520f, 0.0021f },
    { 0.8425f, 0.9154f, 0.0018f }, { 0.9163f, 0.8700f, 0.0017f }, { 0.9786f, 0.8163f, 0.0014f },
    { 1.0263f, 0.7570f, 0.0011f }, { 1.0567f, 0.6949f, 0.0010f }, { 1.0622f, 0.6310f, 0.0008f },
    { 1.0456f, 0.5668f, 0.0006f }, { 1.0026f, 0.5030f, 0.0003f }, { 0.9384f, 0.4412f, 0.0002f },
    { 0.8544f, 0.3810f, 0.0002f }, { 0.7514f, 0.3210f, 0.0001f }, { 0.6424f, 0.2650f, 0.0000f },
    { 0.5419f, 0.2170f, 0.0000f }, { 0.4479f, 0.1750f, 0.0000f }, { 0.3608f, 0.1382f, 0.0000f },
    { 0.2835f, 0.1070f, 0.0000f }, { 0.2187f, 0.0816f, 0.0000f }, { 0.1649f, 0.0610f, 0.0000f },
    { 0.1212f, 0.0446f, 0.0000f }, { 0.0874f, 0.0320f, 0.0000f }, { 0.0636f, 0.0232f, 0.0000f },
    { 0.0468f, 0.0170f, 0.0000f }, { 0.0329f, 0.0119f, 0.0000f }, { 0.0227f, 0.0082f, 0.0000f },
    { 0.0158f, 0.0057f, 0.0000f }, { 0.0114f, 0.0041f, 0.0000f }, { 0.0081f, 0.0029f, 0.0000f },
    { 0.0058f, 0.0021f, 0.0000f }, { 0.0041f, 0.0015f, 0.0000f }, { 0.0029f, 0.0010f, 0.0000f },
    { 0.0020f, 0.0007f, 0.0000f }, { 0.0014f, 0.0005f, 0.0000f }, { 0.0010f, 0.0004f, 0.0000f },
    { 0.0007f, 0.0002f, 0.0000f }, { 0.0005f, 0.0002f, 0.0000f }, { 0.0003f, 0.0001f, 0.0000f },
    { 0.0002f, 0.0001f, 0.0000f }, { 0.0002f, 0.0001f, 0.0000f }, { 0.0001f, 0.0000f, 0.0000f },
    { 0.0001f, 0.0000f, 0.0000f }, { 0.0001f, 0.0000f, 0.0000f } // 780nm is just {0,0,0}
};


/**
 * Given 1976 coordinates u', v', determine 1931 chromaticities x, y.
 * @param {float}  up
 * @param {float}  vp
 * @param {float*} xc
 * @param {float*} yc
 */
inline void upvp_to_xy( float up, float vp, float* xc, float* yc ) {
	*xc = native_divide( 9.0f * up, 6.0f * up - 16.0f * vp + 12.0f );
	*yc = native_divide( 4.0f * vp, 6.0f * up - 16.0f * vp + 12.0f );
}


/**
 * Given 1931 chromaticities x, y, determine 1976 coordinates u', v'.
 * @param {float}  xc
 * @param {float}  yc
 * @param {float*} up
 * @param {float*} vp
 */
inline void xy_to_upvp( float xc, float yc, float* up, float* vp ) {
	*up = native_divide( 4.0f * xc, -2.0f * xc + 12.0f * yc + 3.0f );
	*vp = native_divide( 9.0f * yc, -2.0f * xc + 12.0f * yc + 3.0f );
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
	float xr, yr, zr, xg, yg, zg, xb, yb, zb;
	float xw, yw, zw;
	float rx, ry, rz, gx, gy, gz, bx, by, bz;
	float rw, gw, bw;

	xr = cs.xRed;
	yr = cs.yRed;
	zr = 1.0f - ( xr + yr );

	xg = cs.xGreen;
	yg = cs.yGreen;
	zg = 1.0f - ( xg + yg );

	xb = cs.xBlue;
	yb = cs.yBlue;
	zb = 1.0f - ( xb + yb );

	xw = cs.xWhite;
	yw = cs.yWhite;
	zw = 1.0f - ( xw + yw );

	// TODO: Essentially, these are matrix multiplications. I bet this can be optimized.

	// xyz -> rgb matrix, before scaling to white.

	rx = yg * zb - yb * zg;
	ry = xb * zg - xg * zb;
	rz = xg * yb - xb * yg;

	gx = yb * zr - yr * zb;
	gy = xr * zb - xb * zr;
	gz = xb * yr - xr * yb;

	bx = yr * zg - yg * zr;
	by = xg * zr - xr * zg;
	bz = xr * yg - xg * yr;


	// White scaling factors.
	rw = rx * xw + ry * yw + rz * zw;
	gw = gx * xw + gy * yw + gz * zw;
	bw = bx * xw + by * yw + bz * zw;

	// Dividing by yw scales the white luminance to unity, as conventional.
	rw /= yw;
	gw /= yw;
	bw /= yw;


	// xyz -> rgb matrix, correctly scaled to white.

	rx = native_divide( rx, rw );
	ry = native_divide( ry, rw );
	rz = native_divide( rz, rw );

	gx = native_divide( gx, gw );
	gy = native_divide( gy, gw );
	gz = native_divide( gz, gw );

	bx = native_divide( bx, bw );
	by = native_divide( by, bw );
	bz = native_divide( bz, bw );


	// rgb of the desired point.

	rgb->x = rx * xyz.x + ry * xyz.y + rz * xyz.z;
	rgb->y = gx * xyz.x + gy * xyz.y + gz * xyz.z;
	rgb->z = bx * xyz.x + by * xyz.y + bz * xyz.z;
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
void spectrum_to_xyz( float spd[40], float4* xyz ) {
	float me;
	float X = 0.0f,
	      Y = 0.0f,
	      Z = 0.0f;
	float lambda = 380.0f;

	for( int i = 0, j = 0; lambda < 780.0f; i += 2, j++, lambda += 10.0f ) {
		me = spd[j];
		X += me * cie_colour_match[i][0];
		Y += me * cie_colour_match[i][1];
		Z += me * cie_colour_match[i][2];
	}

	float XYZrecip = native_recip( X + Y + Z );
	xyz->x = X * XYZrecip;
	xyz->y = Y * XYZrecip;
	xyz->z = 1.0f - xyz->x - xyz->y;
}


/**
 * Convert a given spectrum into RGB.
 * @param  {float*} spd Spectral power distribution.
 * @return {float4}     RGB.
 */
float4 spectrumToRGB( float spd[40] ) {
	float4 rgb = (float4)( 0.0f );
	float4 xyz = (float4)( 0.0f );

	spectrum_to_xyz( spd, &xyz );
	xyz_to_rgb( CS, xyz, &rgb );

	if( !inside_gamut( rgb ) ) {
		constrain_rgb( &rgb );
	}

	float m = fmax( rgb.x, fmax( rgb.y, rgb.z ) );
	rgb = ( m > 0.0f ) ? rgb / m : rgb;

	return rgb;
}
