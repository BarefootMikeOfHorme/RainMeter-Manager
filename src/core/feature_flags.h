#pragma once

// Centralized feature flags for conditional compilation
// Defaults prioritize SkiaSharp and disable optional/unstable surfaces.
// Can be overridden via compiler definitions (e.g., /D RM_ENABLE_WEBVIEW2=1).

#ifndef RM_ENABLE_SKIA_SHARP
#define RM_ENABLE_SKIA_SHARP 1
#endif

#ifndef RM_ENABLE_WEBVIEW2
#define RM_ENABLE_WEBVIEW2 0
#endif

#ifndef RM_ENABLE_COMMUNITY_WIDGETS
#define RM_ENABLE_COMMUNITY_WIDGETS 0
#endif

#ifndef RM_ENABLE_GL
#define RM_ENABLE_GL 0
#endif
