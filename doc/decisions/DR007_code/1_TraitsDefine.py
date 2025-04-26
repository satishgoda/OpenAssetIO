# Traits - Modular units of behavior
class GeometryTrait:
    """Defines geometric properties."""
    def __init__(self, vertices, edges, faces):
        self.vertices = vertices
        self.edges = edges
        self.faces = faces

class ShaderTrait:
    """Defines shading properties."""
    def __init__(self, shader_type, parameters):
        self.shader_type = shader_type  # e.g., "PBR", "Phong"
        self.parameters = parameters    # Dictionary of shader parameters

class RiggingTrait:
    """Defines rigging properties."""
    def __init__(self, bones, constraints):
        self.bones = bones              # List of bones
        self.constraints = constraints  # Dictionary of constraints

class AnimationTrait:
    """Defines animation properties."""
    def __init__(self, keyframes, duration):
        self.keyframes = keyframes      # Keyframe data
        self.duration = duration        # Total animation duration

class TextureTrait:
    """Defines texturing properties."""
    def __init__(self, texture_maps):
        self.texture_maps = texture_maps  # Dictionary of texture maps (e.g., "diffuse": "path/to/file")

class AssemblyTrait:
    """Defines assembly properties."""
    def __init__(self, components):
        self.components = components    # List of asset components
