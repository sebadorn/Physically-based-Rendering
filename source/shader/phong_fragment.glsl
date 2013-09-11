uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;
// uniform float shininess;

varying vec3 N;
varying vec3 v;


void main( void ) {
	vec4 diffuseV;
	vec4 specV;
	vec4 ambientV;
	vec4 light = vec4( 0.0, 10.0, 0.0, 1.0 );

	vec3 L = normalize( light - v );
	vec3 E = normalize( -v );
	vec3 R = normalize( reflect( -L, N ) );

	ambientV = ambient;
	diffuseV = clamp( diffuse * max( dot( N, L ), 0.0 ), 0.0, 1.0 ) ;
	specV = clamp ( specular * pow( max( dot( R, E ), 0.0 ), 0.3 * 40.0 ), 0.0, 1.0 );

	gl_FragColor = ambientV + diffuseV + specV;
}
