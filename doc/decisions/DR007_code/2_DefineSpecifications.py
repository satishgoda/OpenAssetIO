# Specifications - Compositions of traits
class ModelSpecification:
    """Specification for a 3D model."""
    def __init__(self, geometry_trait):
        self.geometry = geometry_trait

class ShadingSpecification:
    """Specification for shading."""
    def __init__(self, shader_trait):
        self.shader = shader_trait

class RiggingSpecification:
    """Specification for rigging."""
    def __init__(self, geometry_trait, rigging_trait):
        self.geometry = geometry_trait
        self.rigging = rigging_trait

class AnimationSpecification:
    """Specification for animation."""
    def __init__(self, rigging_trait, animation_trait):
        self.rigging = rigging_trait
        self.animation = animation_trait

class TexturingSpecification:
    """Specification for texturing."""
    def __init__(self, geometry_trait, texture_trait):
        self.geometry = geometry_trait
        self.texture = texture_trait

class FinalAssemblySpecification:
    """Specification for final asset assembly."""
    def __init__(self, assembly_trait):
        self.assembly = assembly_trait
