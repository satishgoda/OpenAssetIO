# Define Traits
geometry_trait = GeometryTrait(vertices=1000, edges=2000, faces=500)
shader_trait = ShaderTrait(shader_type="PBR", parameters={"roughness": 0.5, "metallic": 0.2})
rigging_trait = RiggingTrait(bones=["spine", "arm", "leg"], constraints={"arm": "IK", "leg": "IK"})
animation_trait = AnimationTrait(keyframes=[{"frame": 1, "pose": "A"}, {"frame": 24, "pose": "B"}], duration=1.0)
texture_trait = TextureTrait(texture_maps={"diffuse": "textures/diffuse.png", "normal": "textures/normal.png"})
assembly_trait = AssemblyTrait(components=["Model", "Shader", "Rig", "Animation", "Texture"])

# Create Specifications
model_spec = ModelSpecification(geometry_trait=geometry_trait)
shading_spec = ShadingSpecification(shader_trait=shader_trait)
rigging_spec = RiggingSpecification(geometry_trait=geometry_trait, rigging_trait=rigging_trait)
animation_spec = AnimationSpecification(rigging_trait=rigging_trait, animation_trait=animation_trait)
texturing_spec = TexturingSpecification(geometry_trait=geometry_trait, texture_trait=texture_trait)
final_assembly_spec = FinalAssemblySpecification(assembly_trait=assembly_trait)

# Output Usage
print("Model Specification:", vars(model_spec.geometry))
print("Shading Specification:", vars(shading_spec.shader))
print("Rigging Specification:", vars(rigging_spec.rigging))
print("Animation Specification:", vars(animation_spec.animation))
print("Texturing Specification:", vars(texturing_spec.texture))
print("Final Assembly Specification:", vars(final_assembly_spec.assembly))
