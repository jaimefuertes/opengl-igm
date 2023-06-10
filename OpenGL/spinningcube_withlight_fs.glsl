#version 330

struct Material {
  sampler2D diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out vec4 frag_col;

in vec3 frag_3Dpos;
in vec3 vs_normal;
in vec2 tex_coord;

uniform Material material;
uniform Light light1;
uniform Light light2;
uniform vec3 view_pos;

void main() {
  vec3 light_dir1 = normalize(light1.position - frag_3Dpos);
  vec3 light_dir2 = normalize(light2.position - frag_3Dpos);
  
  // Ambient
  vec3 ambientTex = vec3(texture(material.diffuse, tex_coord));
  
  vec3 ambient1 = light1.ambient * ambientTex;
  vec3 ambient2 = light2.ambient * ambientTex;
  
  vec3 ambient = ambient1 + ambient2;
  
  
  // Diffuse map
  
  float diff1 = max(dot(vs_normal, light_dir1), 0.0);
  float diff2 = max(dot(vs_normal, light_dir2), 0.0);
  
  vec3 diffTex = vec3(texture(material.diffuse, tex_coord));
  
  vec3 diffuse1 = light1.diffuse * diff1 * diffTex;
  vec3 diffuse2 = light2.diffuse * diff2 * diffTex;
  
  vec3 diffuse = diffuse1 + diffuse2;

  // Specular
  vec3 view_dir = normalize(view_pos - frag_3Dpos);
  
  vec3 reflect_dir1 = reflect(-light_dir1, vs_normal);
  vec3 reflect_dir2 = reflect(-light_dir2, vs_normal);
  
  float spec1 = pow(max(dot(view_dir, reflect_dir1), 0.0), material.shininess);
  float spec2 = pow(max(dot(view_dir, reflect_dir2), 0.0), material.shininess);
  
  
  vec3 specular1 = light1.specular * spec1 * material.specular;
  vec3 specular2 = light1.specular * spec2 * material.specular;
  
  vec3 specular = specular1 + specular2;

  vec3 result = ambient + diffuse + specular;
  frag_col = vec4(result, 1.0);
}
