
in vec3 normal;
out vec4 fragColor;

const vec3 incident = normalize(vec3(1.0, 0.2, 1.0));
const vec4 sunColor = vec4(vec3(1.0, 0.95, 0.9) * 1.2, 1.0);
const vec4 skyColor = vec4(vec3(0.9, 0.95, 1.0) * 1.1, 1.0);

void main() {
  vec4 color = vec4(0.9, 0.9, 0.9, 1.0);
  float sunAmount = max(0, dot(normal,  incident));
  float skyAmount = 0.33 * (1.0 - sunAmount);
  color *= (sunAmount * sunColor) + (skyAmount * skyColor);
  fragColor = color;
}
