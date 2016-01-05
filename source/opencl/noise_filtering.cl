#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define NUM_N_SAMPLES 3025
#define NUM_SAMPLES 8
#define SAMPLER CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST
#define STD_DEVIATION_FEATURE 3.0f
#define STD_DEVIATION_WORLD 50.0f


typedef struct {
	const global float4* intersectFirst;
	const global float4* intersectSecond;
	const global float4* normalsFirst;
	const global float4* normalsSecond;
	const global float4* textureFirst;
} samplesData_t;


/**
 * Get a pseudo-random value.
 * @param  {float*} seed
 * @return {float}       A random number [0, 1].
 */
inline float rand( float* seed ) {
	float i;
	*seed += 1.0f;

	return fract( native_sin( *seed ) * 43758.5453123f, &i );
}


/**
 * Check a feature if it can be added to the neighborhood samples.
 * @param  {const float} ftVal  Value of the feature.
 * @param  {const float} m      Mean.
 * @param  {const float} s      Standard deviation of the feature.
 * @param  {const float} stdDev Standard deviation of the current "main" pixel.
 * @return {const char}
 */
const char checkFeature( const float ftVal, const float m, const float s, const float stdDev ) {
	const float delta = fabs( ftVal - m );

	return ( ( delta >= 0.1f || s >= 0.1f ) && delta > stdDev * s );
}


/**
 * Check if the features of the sample can be added to the neighborhood sample set.
 * @param  {const int}            sampleIndex
 * @param  {const samplesData_t*} samplesData
 * @param  {const float*}         mean
 * @param  {const float*}         sigma
 * @return {const char}
 */
const char checkFeatures(
	const int sampleIndex,
	const samplesData_t* samplesData,
	const float* mean,
	const float* sigma
) {
	const float4 pos1 = samplesData->intersectFirst[sampleIndex];
	if( !checkFeature( pos1.x, mean[0], sigma[0], STD_DEVIATION_WORLD ) ) { return 0; }
	if( !checkFeature( pos1.y, mean[1], sigma[1], STD_DEVIATION_WORLD ) ) { return 0; }
	if( !checkFeature( pos1.z, mean[2], sigma[2], STD_DEVIATION_WORLD ) ) { return 0; }

	const float4 pos2 = samplesData->intersectSecond[sampleIndex];
	if( !checkFeature( pos2.x, mean[3], sigma[3], STD_DEVIATION_WORLD ) ) { return 0; }
	if( !checkFeature( pos2.y, mean[4], sigma[4], STD_DEVIATION_WORLD ) ) { return 0; }
	if( !checkFeature( pos2.z, mean[5], sigma[5], STD_DEVIATION_WORLD ) ) { return 0; }

	const float4 n1 = samplesData->normalsFirst[sampleIndex];
	if( !checkFeature( n1.x, mean[6], sigma[6], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( n1.y, mean[7], sigma[7], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( n1.z, mean[8], sigma[8], STD_DEVIATION_FEATURE ) ) { return 0; }

	const float4 n2 = samplesData->normalsSecond[sampleIndex];
	if( !checkFeature( n2.x, mean[9], sigma[9], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( n2.y, mean[10], sigma[10], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( n2.z, mean[11], sigma[11], STD_DEVIATION_FEATURE ) ) { return 0; }

	const float4 tex = samplesData->textureFirst[sampleIndex];
	if( !checkFeature( tex.x, mean[12], sigma[12], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( tex.y, mean[13], sigma[13], STD_DEVIATION_FEATURE ) ) { return 0; }
	if( !checkFeature( tex.z, mean[14], sigma[14], STD_DEVIATION_FEATURE ) ) { return 0; }

	return 1;
}


/**
 * Pre-process the samples of the neighborhood of the current pixel.
 * @param  {const uint}           pxIndex
 * @param  {uint}                 b
 * @param  {float*}               seed
 * @param  {const samplesData_t*} samplesData
 * @return {float*}
 */
float* preprocessSamples(
	const uint pxIndex,
	uint b,
	float* seed,
	const samplesData_t* samplesData
) {
	// For the samples belonging to the current pixel:
	// - Calculate mean.
	// - Calculate standard deviation.

	float mean[15] = { 0.0f };
	float sigma[15] = { 0.0f };

	for( int i = 1; i <= NUM_SAMPLES; i++ ) {
		const float4 pos1 = samplesData->intersectFirst[i * pxIndex];
		const float4 pos2 = samplesData->intersectSecond[i * pxIndex];
		const float4 n1 = samplesData->normalsFirst[i * pxIndex];
		const float4 n2 = samplesData->normalsSecond[i * pxIndex];
		const float4 tex = samplesData->textureFirst[i * pxIndex];

		mean[0] += pos1.x;
		mean[1] += pos1.y;
		mean[2] += pos1.z;

		mean[3] += pos2.x;
		mean[4] += pos2.y;
		mean[5] += pos2.z;

		mean[6] += n1.x;
		mean[7] += n1.y;
		mean[8] += n1.z;

		mean[9] += n2.x;
		mean[10] += n2.y;
		mean[11] += n2.z;

		mean[12] += tex.x;
		mean[13] += tex.y;
		mean[14] += tex.z;
	}

	float recipNum = native_recip( (float) NUM_SAMPLES );
	mean[0] *= recipNum;
	mean[1] *= recipNum;
	mean[2] *= recipNum;
	mean[3] *= recipNum;
	mean[4] *= recipNum;
	mean[5] *= recipNum;
	mean[6] *= recipNum;
	mean[7] *= recipNum;
	mean[8] *= recipNum;
	mean[9] *= recipNum;
	mean[10] *= recipNum;
	mean[11] *= recipNum;
	mean[12] *= recipNum;
	mean[13] *= recipNum;
	mean[14] *= recipNum;

	for( int i = 1; i <= NUM_SAMPLES; i++ ) {
		const float4 pos1 = samplesData->intersectFirst[i * pxIndex];
		const float4 pos2 = samplesData->intersectSecond[i * pxIndex];
		const float4 n1 = samplesData->normalsFirst[i * pxIndex];
		const float4 n2 = samplesData->normalsSecond[i * pxIndex];
		const float4 tex = samplesData->textureFirst[i * pxIndex];

		float s;

		s = pos1.x - mean[0]; sigma[0] += s * s;
		s = pos1.y - mean[1]; sigma[1] += s * s;
		s = pos1.z - mean[2]; sigma[2] += s * s;

		s = pos2.x - mean[3]; sigma[3] += s * s;
		s = pos2.y - mean[4]; sigma[4] += s * s;
		s = pos2.z - mean[5]; sigma[5] += s * s;

		s = n1.x - mean[6]; sigma[6] += s * s;
		s = n1.y - mean[7]; sigma[7] += s * s;
		s = n1.z - mean[8]; sigma[8] += s * s;

		s = n2.x - mean[9]; sigma[9] += s * s;
		s = n2.y - mean[10]; sigma[10] += s * s;
		s = n2.z - mean[11]; sigma[11] += s * s;

		s = tex.x - mean[12]; sigma[12] += s * s;
		s = tex.y - mean[13]; sigma[13] += s * s;
		s = tex.z - mean[14]; sigma[14] += s * s;
	}

	float scalar = native_recip( NUM_SAMPLES - 1.0f );
	sigma[0] = native_sqrt( scalar * sigma[0] );
	sigma[1] = native_sqrt( scalar * sigma[1] );
	sigma[2] = native_sqrt( scalar * sigma[2] );
	sigma[3] = native_sqrt( scalar * sigma[3] );
	sigma[4] = native_sqrt( scalar * sigma[4] );
	sigma[5] = native_sqrt( scalar * sigma[5] );
	sigma[6] = native_sqrt( scalar * sigma[6] );
	sigma[7] = native_sqrt( scalar * sigma[7] );
	sigma[8] = native_sqrt( scalar * sigma[8] );
	sigma[9] = native_sqrt( scalar * sigma[9] );
	sigma[10] = native_sqrt( scalar * sigma[10] );
	sigma[11] = native_sqrt( scalar * sigma[11] );
	sigma[12] = native_sqrt( scalar * sigma[12] );
	sigma[13] = native_sqrt( scalar * sigma[13] );
	sigma[14] = native_sqrt( scalar * sigma[14] );


	// Select the neighborhood samples.

	int N[NUM_N_SAMPLES]; // 55 * 55
	int lengthN = 0;

	// Add all samples of the current pixel.
	#pragma unroll(NUM_SAMPLES)
	for( int i = 1; i <= NUM_SAMPLES; i++ ) {
		N[lengthN++] = pxIndex * i;
	}

	// Randomly pick samples of the pixels close to the current one.
	// But only if they are within the standard deviation.
	const int start = fmax( pxIndex - ( b - 1 ) * 0.5, 0 );
	const int endX = fmin( pxIndex + ( b - 1 ) * 0.5, IMG_WIDTH - 1 );
	const int endY = fmin( pxIndex + ( b - 1 ) * 0.5, IMG_HEIGHT - 1 );

	for( int y = start; y < endY; y++ ) {
		for( int x = start; x < endX; x++ ) {
			const int xy = x + y * IMG_WIDTH;

			if( xy >= IMG_WIDTH * IMG_HEIGHT ) {
				break;
			}

			if( xy == pxIndex ) {
				continue;
			}

			const int randSample = round( rand( seed ) * NUM_SAMPLES );
			const int sampleIndex = randSample * xy;
			const char keepSample = checkFeatures( sampleIndex, samplesData, mean, sigma );

			if( keepSample == 1 ) {
				N[lengthN++] = sampleIndex;
			}
		}
	}


	// Compute mean and standard deviation for the neighborhood samples.

	float nMean[15];
	float nSigma[15];

	for( int i = 0; i < lengthN; i++ ) {
		const int sampleIndex = N[i];
		const float4 pos1 = samplesData->intersectFirst[sampleIndex];
		const float4 pos2 = samplesData->intersectSecond[sampleIndex];
		const float4 n1 = samplesData->normalsFirst[sampleIndex];
		const float4 n2 = samplesData->normalsSecond[sampleIndex];
		const float4 tex = samplesData->textureFirst[sampleIndex];

		nMean[0] += pos1.x;
		nMean[1] += pos1.y;
		nMean[2] += pos1.z;

		nMean[3] += pos2.x;
		nMean[4] += pos2.y;
		nMean[5] += pos2.z;

		nMean[6] += n1.x;
		nMean[7] += n1.y;
		nMean[8] += n1.z;

		nMean[9] += n2.x;
		nMean[10] += n2.y;
		nMean[11] += n2.z;

		nMean[12] += tex.x;
		nMean[13] += tex.y;
		nMean[14] += tex.z;
	}

	recipNum = native_recip( (float) lengthN );
	nMean[0] *= recipNum;
	nMean[1] *= recipNum;
	nMean[2] *= recipNum;
	nMean[3] *= recipNum;
	nMean[4] *= recipNum;
	nMean[5] *= recipNum;
	nMean[6] *= recipNum;
	nMean[7] *= recipNum;
	nMean[8] *= recipNum;
	nMean[9] *= recipNum;
	nMean[10] *= recipNum;
	nMean[11] *= recipNum;
	nMean[12] *= recipNum;
	nMean[13] *= recipNum;
	nMean[14] *= recipNum;

	for( int i = 0; i < lengthN; i++ ) {
		const int sampleIndex = N[i];
		const float4 pos1 = samplesData->intersectFirst[sampleIndex];
		const float4 pos2 = samplesData->intersectSecond[sampleIndex];
		const float4 n1 = samplesData->normalsFirst[sampleIndex];
		const float4 n2 = samplesData->normalsSecond[sampleIndex];
		const float4 tex = samplesData->textureFirst[sampleIndex];

		float s;

		s = pos1.x - nMean[0]; nSigma[0] += s * s;
		s = pos1.y - nMean[1]; nSigma[1] += s * s;
		s = pos1.z - nMean[2]; nSigma[2] += s * s;

		s = pos2.x - nMean[3]; nSigma[3] += s * s;
		s = pos2.y - nMean[4]; nSigma[4] += s * s;
		s = pos2.z - nMean[5]; nSigma[5] += s * s;

		s = n1.x - nMean[6]; nSigma[6] += s * s;
		s = n1.y - nMean[7]; nSigma[7] += s * s;
		s = n1.z - nMean[8]; nSigma[8] += s * s;

		s = n2.x - nMean[9]; nSigma[9] += s * s;
		s = n2.y - nMean[10]; nSigma[10] += s * s;
		s = n2.z - nMean[11]; nSigma[11] += s * s;

		s = tex.x - nMean[12]; nSigma[12] += s * s;
		s = tex.y - nMean[13]; nSigma[13] += s * s;
		s = tex.z - nMean[14]; nSigma[14] += s * s;
	}

	scalar = native_recip( lengthN - 1.0f );
	nSigma[0] = native_sqrt( scalar * nSigma[0] );
	nSigma[1] = native_sqrt( scalar * nSigma[1] );
	nSigma[2] = native_sqrt( scalar * nSigma[2] );
	nSigma[3] = native_sqrt( scalar * nSigma[3] );
	nSigma[4] = native_sqrt( scalar * nSigma[4] );
	nSigma[5] = native_sqrt( scalar * nSigma[5] );
	nSigma[6] = native_sqrt( scalar * nSigma[6] );
	nSigma[7] = native_sqrt( scalar * nSigma[7] );
	nSigma[8] = native_sqrt( scalar * nSigma[8] );
	nSigma[9] = native_sqrt( scalar * nSigma[9] );
	nSigma[10] = native_sqrt( scalar * nSigma[10] );
	nSigma[11] = native_sqrt( scalar * nSigma[11] );
	nSigma[12] = native_sqrt( scalar * nSigma[12] );
	nSigma[13] = native_sqrt( scalar * nSigma[13] );
	nSigma[14] = native_sqrt( scalar * nSigma[14] );


	// Normalize neighborhood samples.

	float samples[NUM_N_SAMPLES * 15];
	int samplesLength = 0;

	for( int i = 0; i < lengthN; i++ ) {
		const int sampleIndex = N[i];
		const float4 pos1 = samplesData->intersectFirst[sampleIndex];
		const float4 pos2 = samplesData->intersectSecond[sampleIndex];
		const float4 n1 = samplesData->normalsFirst[sampleIndex];
		const float4 n2 = samplesData->normalsSecond[sampleIndex];
		const float4 tex = samplesData->textureFirst[sampleIndex];

		samples[samplesLength++] = native_divide( pos1.x - nMean[0], sigma[0] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[1], sigma[1] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[2], sigma[2] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[3], sigma[3] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[4], sigma[4] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[5], sigma[5] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[6], sigma[6] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[7], sigma[7] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[8], sigma[8] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[9], sigma[9] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[10], sigma[10] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[11], sigma[11] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[12], sigma[12] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[13], sigma[13] );
		samples[samplesLength++] = native_divide( pos1.x - nMean[14], sigma[14] );
	}

	return samples;
}


/**
 * Compute the feature weights.
 * @param {const int}           x
 * @param {const int}           y
 * @param {const uint}          pxIndex
 * @param {float*}              neighbordhoodSamples
 * @param {read_only image2d_t} imageIn
 */
void computeFeatureWeights(
	const int x,
	const int y,
	const uint pxIndex,
	float* neighbordhoodSamples,
	read_only image2d_t imageIn
) {
	float alphaWeight = 0.0f;
	float betaWeight = 0.0f;

	// TODO: calculate statistical dependencies
	// TODO: calculate alpha weight
	// TODO: calculate beta weight
}


/**
 * One filter pass.
 * @param {uint}                 blockWidth
 * @param {const samplesData_t*} samplesData
 * @param {read_only image2d_t}  imageIn
 * @param {write_only image2d_t} imageOut
 */
void filter_pass(
	uint blockWidth,
	float* seed,
	const samplesData_t* samplesData,
	read_only image2d_t imageIn,
	write_only image2d_t imageOut
) {
	// Iterate all pixels.
	for( int y = 0; y < IMG_HEIGHT; y++ ) {
		for( int x = 0; x < IMG_WIDTH; x++ ) {
			const uint pxIndex = x + y * IMG_WIDTH;

			float* neighbordhoodSamples = preprocessSamples( pxIndex, blockWidth, seed, samplesData );

			// float4 pxColor = read_imagef( imageIn, SAMPLER, (int2)( x, y ) );
			computeFeatureWeights( x, y, pxIndex, neighbordhoodSamples, imageIn );
			// TODO: filter color samples
		}
	}
}


/**
 * Apply a noise filter to the last rendered image.
 * @param {const global float4*} intersectFirst
 * @param {const global float4*} intersectSecond
 * @param {const global float4*} normalsFirst
 * @param {const global float4*} normalsSecond
 * @param {const global float4*} textureFirst
 * @param {read_only image2d_t}  imageIn
 * @param {write_only image2d_t} imageOut
 */
kernel void noise_filtering(
	float seed,

	// vector of the first intersection
	const global float4* intersectFirst,
	// vector of the second intersection
	const global float4* intersectSecond,
	// normals of the first intersection
	const global float4* normalsFirst,
	// normals of the second intersection
	const global float4* normalsSecond,
	// texture color of the first intersection
	const global float4* textureFirst,

	read_only image2d_t imageIn,
	write_only image2d_t imageOut
) {
	samplesData_t samplesData = {
		intersectFirst, intersectSecond,
		normalsFirst, normalsSecond,
		textureFirst
	};

	filter_pass( 55, &seed, &samplesData, imageIn, imageOut );
	filter_pass( 35, &seed, &samplesData, imageIn, imageOut );
	filter_pass( 17, &seed, &samplesData, imageIn, imageOut );
	filter_pass( 7, &seed, &samplesData, imageIn, imageOut );
}