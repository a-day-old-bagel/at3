uniform sampler2D texture0;

in vec2 texCoord;
out vec4 fragColor;

void main() {
  fragColor = vec4(texture(texture0, texCoord).rgb, 1.0);
}
