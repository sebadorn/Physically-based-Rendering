uniform vec4 ambientMat;
uniform vec4 diffuseMat;
uniform vec4 specMat;
uniform float specPow;

varying vec3 N;
varying vec3 v;

void main( void ) {
	v = vec3( gl_ModelViewMatrix * gl_Vertex );
	N = normalize( gl_NormalMatrix * gl_Normal );
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
