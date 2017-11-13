uniform sampler2D texture0;

in vec2 texCoord;
out vec4 fragColor;

void main() {
    float alpha = texture(texture0, texCoord).a / 100.0;
    fragColor = vec4(alpha, alpha, alpha, 1.0);
//    fragColor = vec4(texture(texture0, texCoord).rgb, 1.0);
}
