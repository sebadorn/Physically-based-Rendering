/*
                Colour Rendering of Spectra

                       by John Walker
                  http://www.fourmilab.ch/

		 Last updated: March 9, 2003

           This program is in the public domain.

    For complete information about the techniques employed in
    this program, see the World-Wide Web document:

             http://www.fourmilab.ch/documents/specrend/

    The xyz_to_rgb() function, which was wrong in the original
    version of this program, was corrected by:

	    Andrew J. S. Hamilton 21 May 1999
	    Andrew.Hamilton@Colorado.EDU
	    http://casa.colorado.edu/~ajsh/

    who also added the gamma correction facilities and
    modified constrain_rgb() to work by desaturating the
    colour by adding white.

    A program which uses these functions to plot CIE
    "tongue" diagrams called "ppmcie" is included in
    the Netpbm graphics toolkit:
    	http://netpbm.sourceforge.net/
    (The program was called cietoppm in earlier
    versions of Netpbm.)
*/

/* A colour system is defined by the CIE x and y coordinates of
   its three primary illuminants and the x and y coordinates of
   the white point. */
typedef struct {
	float xRed, yRed,     /* Red x, y */
	      xGreen, yGreen, /* Green x, y */
	      xBlue, yBlue,   /* Blue x, y */
	      xWhite, yWhite, /* White point x, y */
	      gamma;          /* Gamma correction for system */
} colourSystem;

/* White point chromaticities. */
#define IlluminantC   0.3101f, 0.3162f         /* For NTSC television */
#define IlluminantD65 0.3127f, 0.3291f         /* For EBU and SMPTE */
#define IlluminantE   0.33333333f, 0.33333333f /* CIE equal-energy illuminant */

/* Gamma of nonlinear correction.
   See Charles Poynton's ColorFAQ Item 45 and GammaFAQ Item 6 at:
     http://www.poynton.com/ColorFAQ.html
     http://www.poynton.com/GammaFAQ.html
*/
#define GAMMA_REC709 0.0f /* Rec. 709 */

constant colourSystem
                  /* xRed     yRed     xGreen   yGreen   xBlue    yBlue    White point    Gamma   */
	NTSCsystem   = { 0.67f,   0.33f,   0.21f,   0.71f,   0.14f,   0.08f,   IlluminantC,   GAMMA_REC709 },
	EBUsystem    = { 0.64f,   0.33f,   0.29f,   0.60f,   0.15f,   0.06f,   IlluminantD65, GAMMA_REC709 },
	SMPTEsystem  = { 0.630f,  0.340f,  0.310f,  0.595f,  0.155f,  0.070f,  IlluminantD65, GAMMA_REC709 },
	HDTVsystem   = { 0.670f,  0.330f,  0.210f,  0.710f,  0.150f,  0.060f,  IlluminantD65, GAMMA_REC709 },
	CIEsystem    = { 0.7355f, 0.2645f, 0.2658f, 0.7243f, 0.1669f, 0.0085f, IlluminantE,   GAMMA_REC709 },
	Rec709system = { 0.64f,   0.33f,   0.30f,   0.60f,   0.15f,   0.06f,   IlluminantD65, GAMMA_REC709 };

/*  	    	    	    UPVP_TO_XY
    Given 1976 coordinates u', v', determine 1931 chromaticities x, y
*/
void upvp_to_xy( float up, float vp, float* xc, float* yc ) {
	*xc = ( 9.0f * up ) / ( ( 6.0f * up ) - ( 16.0f * vp ) + 12.0f );
	*yc = ( 4.0f * vp ) / ( ( 6.0f * up ) - ( 16.0f * vp ) + 12.0f );
}

/*  	    	    	    XY_TO_UPVP
    Given 1931 chromaticities x, y, determine 1976 coordinates u', v'
*/
void xy_to_upvp( float xc, float yc, float* up, float* vp ) {
	*up = ( 4.0f * xc ) / ( ( -2.0f * xc ) + ( 12.0f * yc ) + 3.0f );
	*vp = ( 9.0f * yc ) / ( ( -2.0f * xc ) + ( 12.0f * yc ) + 3.0f );
}

/*                             XYZ_TO_RGB
    Given an additive tricolour system CS, defined by the CIE x
    and y chromaticities of its three primaries (z is derived
    trivially as 1-(x+y)), and a desired chromaticity (XC, YC,
    ZC) in CIE space, determine the contribution of each
    primary in a linear combination which sums to the desired
    chromaticity.  If the  requested chromaticity falls outside
    the Maxwell  triangle (colour gamut) formed by the three
    primaries, one of the r, g, or b weights will be negative.

    Caller can use constrain_rgb() to desaturate an
    outside-gamut colour to the closest representation within
    the available gamut and/or norm_rgb to normalise the RGB
    components so the largest nonzero component has value 1.

*/
void xyz_to_rgb(
	colourSystem* cs,
	float xc, float yc, float zc,
	float* r, float* g, float* b
) {
	float xr, yr, zr, xg, yg, zg, xb, yb, zb;
	float xw, yw, zw;
	float rx, ry, rz, gx, gy, gz, bx, by, bz;
	float rw, gw, bw;

	xr = cs->xRed;
	yr = cs->yRed;
	zr = 1.0f - ( xr + yr );

	xg = cs->xGreen;
	yg = cs->yGreen;
	zg = 1.0f - ( xg + yg );

	xb = cs->xBlue;
	yb = cs->yBlue;
	zb = 1.0f - ( xb + yb );

	xw = cs->xWhite;
	yw = cs->yWhite;
	zw = 1.0f - ( xw + yw );

	/* xyz -> rgb matrix, before scaling to white. */

	rx = ( yg * zb ) - ( yb * zg );
	ry = ( xb * zg ) - ( xg * zb );
	rz = ( xg * yb ) - ( xb * yg );

	gx = ( yb * zr ) - ( yr * zb );
	gy = ( xr * zb ) - ( xb * zr );
	gz = ( xb * yr ) - ( xr * yb );

	bx = ( yr * zg ) - ( yg * zr );
	by = ( xg * zr ) - ( xr * zg );
	bz = ( xr * yg ) - ( xg * yr );

	/* White scaling factors.
	   Dividing by yw scales the white luminance to unity, as conventional. */

	rw = ( ( rx * xw ) + ( ry * yw ) + ( rz * zw ) ) / yw;
	gw = ( ( gx * xw ) + ( gy * yw ) + ( gz * zw ) ) / yw;
	bw = ( ( bx * xw ) + ( by * yw ) + ( bz * zw ) ) / yw;

	/* xyz -> rgb matrix, correctly scaled to white. */

	rx = rx / rw;
	ry = ry / rw;
	rz = rz / rw;

	gx = gx / gw;
	gy = gy / gw;
	gz = gz / gw;

	bx = bx / bw;
	by = by / bw;
	bz = bz / bw;

	/* rgb of the desired point */

	*r = ( rx * xc ) + ( ry * yc ) + ( rz * zc );
	*g = ( gx * xc ) + ( gy * yc ) + ( gz * zc );
	*b = ( bx * xc ) + ( by * yc ) + ( bz * zc );
}

/*                            INSIDE_GAMUT
     Test whether a requested colour is within the gamut
     achievable with the primaries of the current colour
     system.  This amounts simply to testing whether all the
     primary weights are non-negative. */

int inside_gamut( float r, float g, float b ) {
	return (
		r >= 0.0f &&
		g >= 0.0f &&
		b >= 0.0f
	);
}

/*                          CONSTRAIN_RGB

    If the requested RGB shade contains a negative weight for
    one of the primaries, it lies outside the colour gamut
    accessible from the given triple of primaries.  Desaturate
    it by adding white, equal quantities of R, G, and B, enough
    to make RGB all positive.  The function returns 1 if the
    components were modified, zero otherwise.

*/

int constrain_rgb( float* r, float* g, float* b ) {
	float w;

	/* Amount of white needed is w = - min(0, *r, *g, *b) */

	w = ( 0.0f < *r ) ? 0.0f : *r;
	w = ( w < *g ) ? w : *g;
	w = ( w < *b ) ? w : *b;
	w = -w;

	/* Add just enough white to make r, g, b all positive. */

	if( w > 0.0f ) {
		*r += w;
		*g += w;
		*b += w;

		return 1; /* Colour modified to fit RGB gamut */
	}

	return 0; /* Colour within RGB gamut */
}

/*                          GAMMA_CORRECT_RGB
    Transform linear RGB values to nonlinear RGB values. Rec.
    709 is ITU-R Recommendation BT. 709 (1990) ``Basic
    Parameter Values for the HDTV Standard for the Studio and
    for International Programme Exchange'', formerly CCIR Rec.
    709. For details see
      http://www.poynton.com/ColorFAQ.html
      http://www.poynton.com/GammaFAQ.html
*/

void gamma_correct( const colourSystem* cs, float* c ) {
	float gamma = cs->gamma;

	if( gamma == GAMMA_REC709 ) {
		/* Rec. 709 gamma correction. */
		float cc = 0.018f;

		if( *c < cc ) {
			*c *= ( ( 1.099f * pow( cc, 0.45f ) ) - 0.099f ) / cc;
		}
		else {
			*c = ( 1.099f * pow( *c, 0.45f ) ) - 0.099f;
		}
	}
	else {
		/* Nonlinear colour = (Linear colour)^(1/gamma) */
		*c = pow( *c, 1.0f / gamma );
    }
}

void gamma_correct_rgb( const colourSystem* cs, float* r, float* g, float* b ) {
	gamma_correct( cs, r );
	gamma_correct( cs, g );
	gamma_correct( cs, b );
}

/*  	    	    	    NORM_RGB
    Normalise RGB components so the most intense (unless all
    are zero) has a value of 1.
*/
void norm_rgb( float* r, float* g, float* b ) {
	float greatest = fmax( *r, fmax( *g, *b ) );

	if( greatest > 0.0f ) {
		*r /= greatest;
		*g /= greatest;
		*b /= greatest;
	}
}


/*                            BB_SPECTRUM
    Calculate, by Planck's radiation law, the emittance of a black body
    of temperature bbTemp at the given wavelength (in metres).  */

#define BB_TEMP 5000 /* Hidden temperature argument to BB_SPECTRUM. */

float bb_spectrum( float wavelength ) {
    float wlm = wavelength * 1e-9; /* Wavelength in meters */

    return ( 3.74183e-16 * pow( wlm, -5.0f ) ) / ( exp(1.4388e-2 / ( wlm * BB_TEMP ) ) - 1.0f );
}


/*                          SPECTRUM_TO_XYZ
    Calculate the CIE X, Y, and Z coordinates corresponding to
    a light source with spectral distribution given by the
    function SPEC_INTENS, which is called with a series of
    wavelengths between 380 and 780 nm (the argument is
    expressed in meters), which returns emittance at that
    wavelength in arbitrary units. The chromaticity
    coordinates of the spectrum are returned in the x, y, and z
    arguments which respect the identity:
            x + y + z = 1.
*/
void spectrum_to_xyz( float* x, float* y, float* z ) {
	int i;
	float lambda, X = 0.0f, Y = 0.0f, Z = 0.0f, XYZ;

	/* CIE colour matching functions xBar, yBar, and zBar for
	   wavelengths from 380 through 780 nanometers, every 5
	   nanometers.  For a wavelength lambda in this range:
	        cie_colour_match[(lambda - 380) / 5][0] = xBar
	        cie_colour_match[(lambda - 380) / 5][1] = yBar
	        cie_colour_match[(lambda - 380) / 5][2] = zBar

	To save memory, this table can be declared as floats
	rather than doubles; (IEEE) float has enough
	significant bits to represent the values. It's declared
	as a double here to avoid warnings about "conversion
	between floating-point types" from certain persnickety
	compilers. */

	static float cie_colour_match[81][3] = {
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
	    { 0.0001f, 0.0000f, 0.0000f }, { 0.0001f, 0.0000f, 0.0000f }, { 0.0000f, 0.0000f, 0.0000f }
	};

	for( i = 0, lambda = 380.0f; lambda < 780.1f; i++, lambda += 5.0f ) {
		float me;

		me = bb_spectrum( lambda );
		X += me * cie_colour_match[i][0];
		Y += me * cie_colour_match[i][1];
		Z += me * cie_colour_match[i][2];
	}

	XYZ = ( X + Y + Z );
	*x = X / XYZ;
	*y = Y / XYZ;
	*z = Z / XYZ;
}


/*  Built-in test program which displays the x, y, and Z and RGB
    values for black body spectra from 1000 to 10000 degrees kelvin.
    When run, this program should produce the following output:

    Temperature       x      y      z       R     G     B
    -----------    ------ ------ ------   ----- ----- -----
       1000 K      0.6528 0.3444 0.0028   1.000 0.007 0.000 (Approximation)
       1500 K      0.5857 0.3931 0.0212   1.000 0.126 0.000 (Approximation)
       2000 K      0.5267 0.4133 0.0600   1.000 0.234 0.010
       2500 K      0.4770 0.4137 0.1093   1.000 0.349 0.067
       3000 K      0.4369 0.4041 0.1590   1.000 0.454 0.151
       3500 K      0.4053 0.3907 0.2040   1.000 0.549 0.254
       4000 K      0.3805 0.3768 0.2428   1.000 0.635 0.370
       4500 K      0.3608 0.3636 0.2756   1.000 0.710 0.493
       5000 K      0.3451 0.3516 0.3032   1.000 0.778 0.620
       5500 K      0.3325 0.3411 0.3265   1.000 0.837 0.746
       6000 K      0.3221 0.3318 0.3461   1.000 0.890 0.869
       6500 K      0.3135 0.3237 0.3628   1.000 0.937 0.988
       7000 K      0.3064 0.3166 0.3770   0.907 0.888 1.000
       7500 K      0.3004 0.3103 0.3893   0.827 0.839 1.000
       8000 K      0.2952 0.3048 0.4000   0.762 0.800 1.000
       8500 K      0.2908 0.3000 0.4093   0.711 0.766 1.000
       9000 K      0.2869 0.2956 0.4174   0.668 0.738 1.000
       9500 K      0.2836 0.2918 0.4246   0.632 0.714 1.000
      10000 K      0.2807 0.2884 0.4310   0.602 0.693 1.000
*/


// int main()
// {
//     double t, x, y, z, r, g, b;
//     struct colourSystem *cs = &SMPTEsystem;

//     printf("Temperature       x      y      z       R     G     B\n");
//     printf("-----------    ------ ------ ------   ----- ----- -----\n");

//     for (t = 1000; t <= 10000; t+= 500) {
//         bbTemp = t;
//         spectrum_to_xyz(bb_spectrum, &x, &y, &z);
//         xyz_to_rgb(cs, x, y, z, &r, &g, &b);
//         printf("  %5.0f K      %.4f %.4f %.4f   ", t, x, y, z);
//         if (constrain_rgb(&r, &g, &b)) {
// 	    norm_rgb(&r, &g, &b);
//             printf("%.3f %.3f %.3f (Approximation)\n", r, g, b);
//         } else {
// 	    norm_rgb(&r, &g, &b);
//             printf("%.3f %.3f %.3f\n", r, g, b);
//         }
//     }
//     return 0;
// }
