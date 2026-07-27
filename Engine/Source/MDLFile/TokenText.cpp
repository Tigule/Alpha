namespace MDL {

  static char s_singletoktbl[256][2];
  static const char *s_tokentext[224] = {
    "Literal Long", "Literal Float", "Literal String", "Version", "Model", "Sequences",
    "GlobalSequences", "TextureAnims", "Textures", "Materials", "Geoset", "GeosetAnim",
    "Bone", "Mesh", "Light", "Helper", "Attachment", "PivotPoints", "ParticleEmitter",
    "ParticleEmitter2", "Camera", "EventObject", "HitTestShape", "CollisionShape",
    "RibbonEmitter", "Collision", "Additive", "AddAlpha", "Alpha", "AlphaKey",
    "AlwaysAnimate", "AmbColor", "AmbIntensity", "Ambient", "Anim", "AnimationFile",
    "AttachmentID", "Attenuation", "AttenuationStart", "AttenuationEnd", "Bezier",
    "Billboarded", "BillboardedLockX", "BillboardedLockY", "BillboardedLockZ", "Bitmap",
    "Blend", "BlendColors", "BlendTime", "BoneIndices", "BoneWeights", "Both",
    "BoundsRadius", "Box", "Color", "Columns", "ComponentSkin", "Connect",
    "ConstantColor", "CoordId", "Cylinder", "DecayUVAnim", "Directional", "DontInherit",
    "DontInterp", "Drag", "Duplicates", "Duration", "EmissionRate", "EmitterUsesMDL",
    "EmitterUsesTGA", "EventTrack", "Faces", "FarClip", "FieldOfView", "FilterMode",
    "FormatVersion", "FPS", "Frequency", "FullResolution", "GeosetId", "GeosetAnimId",
    "GlobalSeqId", "Gravity", "GroundTrack", "Group", "Groups", "Head", "Height",
    "HeightAbove", "HeightBelow", "Hermite", "Image", "InitVelocity", "InTan",
    "Intensity", "Interval", "Latitude", "Longitude", "Layer", "Length", "LifeSpan",
    "LifeSpanUVAnim", "Linear", "Lines", "LineEmitter", "LineLoop", "LineStrip",
    "Material", "MaterialID", "Matrices", "MaximumExtent", "MinimumExtent", "ModelSpace",
    "Modulate", "Modulate2x", "MoveSpeed", "Multiple", "NearClip", "NoDepthTest",
    "NoDepthSet", "None", "NonLooping", "Normals", "NumAttachments", "NumBones",
    "NumEvents", "NumGeosets", "NumGeosetAnims", "NumHelpers", "NumLights", "NumMeshes",
    "NumParticleEmitters", "NumParticleEmitters2", "NumRibbonEmitters", "ObjectId",
    "Omnidirectional", "Opacity", "OutTan", "Parent", "Particle", "Particle0XKill",
    "ParticleExtrude", "ParticleFollow", "ParticleFollowParams", "ParticleInheritScale",
    "ParticleIVelLin", "ParticleIVelScale", "ParticleGeometryMdl", "ParticleProject",
    "ParticleRecursionMdl", "ParticleRotation", "ParticleScaling", "ParticleTumble",
    "ParticleTumbleR", "ParticleTwinkleOnOff", "ParticleTwinkleScale", "ParticleXYQuads",
    "ParticleZSource", "ParticleZVelOnly", "Path", "Pitch", "Plane", "Points", "Polygons",
    "Position", "PriorityPlane", "Project", "Quads", "QuadStrip", "ReplaceableId", "Replay",
    "Roll", "Rotation", "Rows", "Scaling", "SegmentColor", "SelectionGroup",
    "MaximumDistance", "MinimumDistance", "Sphere", "SphereEnvMap", "Spline",
    "SortPrimsFarZ", "SortPrimsNearZ", "Speed", "Squirt", "static", "Tail", "TailGrows",
    "TailLength", "TailDecayUVAnim", "TailUVAnim", "Target", "TeamColor", "TextureID",
    "TextureSlot", "TFaces", "Time", "Translation", "Transparent", "Triangles",
    "TriangleFan", "TriangleStrip", "TVertexAnim", "TVertexAnimId", "TVertices", "TwoSided",
    "Type", "Unfogged", "Unselectable", "Unshaded", "Variation", "Vertex", "VertexCount",
    "VertexGroup", "Vertices", "Visibility", "Width", "Wind", "WrapHeight", "WrapWidth",
    "Yaw", "Unknown"
  };

  void InitializeTokenText() {
    unsigned int index = 256;

    do {
      --index;
      s_singletoktbl[index][0] = static_cast<char>(index);
    } while (index);
  }

  void DestroyTokenText() {
  }

  const char *TokenText(unsigned int token) {
    if (token < 256) {
      return s_singletoktbl[token];
    }
    if (token > 479) {
      token = token & 0x80000000 ? 0 : 479;
      if (token < 256) {
        return s_singletoktbl[token];
      }
    }
    return s_tokentext[token - 256];
  }

}  // namespace MDL
