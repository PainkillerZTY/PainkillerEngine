#version 330 core

in vec3 v_Normal;
in vec3 v_FragPos;
in vec3 v_ViewDir;
in vec2 v_TexCoord;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform float u_Time;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_LightDir);
    
    // Ambient
    vec3 ambient = vec3(0.15, 0.15, 0.2);
    
    // Diffuse
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;
    
    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(v_ViewDir);
    vec3 halfDir = normalize(-lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * u_LightColor * 0.5;
    
    // Simple color with checker pattern
    vec3 color = vec3(0.3, 0.5, 0.9);
    float checker = step(0.5, fract(v_FragPos.x * 2.0) * fract(v_FragPos.z * 2.0));
    color = mix(color, color * 1.3, step(0.5, fract(v_FragPos.x * 2.0) * fract(v_FragPos.z * 2.0)));
    color = mix(color, color * 0.7, step(0.5, fract(v_FragPos.x * 2.0 + 1.0) * fract(v_FragPos.z * 2.0 + 1.0)));
    
    vec3 result = (ambient + diffuse + specular) * color;
    FragColor = vec4(result, 1.0);
}
