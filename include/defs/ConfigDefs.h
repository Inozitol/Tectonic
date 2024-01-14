#ifndef TECTONIC_CONFIGDEFS_H
#define TECTONIC_CONFIGDEFS_H

#define SHADOW_WIDTH         3000
#define SHADOW_HEIGHT        3000

#define CAMERA_PPROJ_FOV 60.0
#define CAMERA_PPROJ_NEAR 0.01
#define CAMERA_PPROJ_FAR 150.0

#define SHADOW_SPOT_PPROJ_FOV     45.0
#define SHADOW_SPOT_PPROJ_NEAR    0.1
#define SHADOW_SPOT_PPROJ_FAR     150.0

#define SHADOW_POINT_PPROJ_FOV     90.0
#define SHADOW_POINT_PPROJ_NEAR    0.1
#define SHADOW_POINT_PPROJ_FAR     20.0

#define SHADOW_OPROJ_LEFT   -3.0f
#define SHADOW_OPROJ_RIGHT   3.0f
#define SHADOW_OPROJ_BOTTOM -3.0f
#define SHADOW_OPROJ_TOP     3.0f
#define SHADOW_OPROJ_NEAR   -3.0f
#define SHADOW_OPROJ_FAR     3.0f

#define LIGHTING_VERT_SHADER_PATH   "shaders/vert/lighting.vert"
#define LIGHTING_FRAG_SHADER_PATH   "shaders/frag/lighting.frag"
#define SHADOWMAP_VERT_SHADER_PATH  "shaders/vert/shadow.vert"
#define SHADOWMAP_FRAG_SHADER_PATH  "shaders/frag/shadow.frag"
#define PICKING_VERT_SHADER_PATH    "shaders/vert/picking.vert"
#define PICKING_FRAG_SHADER_PATH    "shaders/frag/picking.frag"
#define DEBUG_VERT_SHADER_PATH      "shaders/vert/debug.vert"
#define DEBUG_GEOM_SHADER_PATH      "shaders/geom/debug.geom"
#define DEBUG_FRAG_SHADER_PATH      "shaders/frag/debug.frag"
#define TERRAIN_VERT_SHADER_PATH    "shaders/vert/terrain.vert"
#define TERRAIN_FRAG_SHADER_PATH    "shaders/frag/terrain.frag"
#define SKYBOX_VERT_SHADER_PATH     "shaders/vert/skybox.vert"
#define SKYBOX_FRAG_SHADER_PATH     "shaders/frag/skybox.frag"

#endif //TECTONIC_CONFIGDEFS_H
