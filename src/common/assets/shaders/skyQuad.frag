precision mediump float;
uniform samplerCube texture0;
varying vec3 eyeDirection;

void main() {
    gl_FragColor = textureCube(texture0, eyeDirection);
}