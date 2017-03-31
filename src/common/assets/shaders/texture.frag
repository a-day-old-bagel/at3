precision mediump float;

uniform sampler2D texture0;

varying vec2 texCoord;

void main() {
  gl_FragColor = vec4(texture2D(texture0, texCoord).rgb, 1.0);
}
