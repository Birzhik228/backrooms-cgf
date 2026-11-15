# Project modification of pinned GLFW 3.4. Hardware rendering remains preferred.
# GitHub's Mac VMs expose Apple's software OpenGL but no accelerated pixel format.
# See https://github.com/glfw/glfw/issues/2570 . Keep the requested core profile.
if(NOT DEFINED GLFW_SOURCE_DIR)
  message(FATAL_ERROR "GLFW_SOURCE_DIR is required")
endif()
set(source "${GLFW_SOURCE_DIR}/src/nsgl_context.m")
file(READ "${source}" content)
set(marker "Backrooms CGF: permit Apple's software renderer after hardware selection fails")
string(FIND "${content}" "${marker}" already_patched)
if(NOT already_patched EQUAL -1)
  return()
endif()
set(expected [=[    window->context.nsgl.pixelFormat =
        [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];]=])
set(replacement [=[    window->context.nsgl.pixelFormat =
        [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    // Backrooms CGF: permit Apple's software renderer after hardware selection fails.
    // attribs[0] is NSOpenGLPFAAccelerated in pinned GLFW 3.4. All framebuffer
    // and core-profile requirements remain intact, and GLFW validates the GL version.
    if (window->context.nsgl.pixelFormat == nil)
        window->context.nsgl.pixelFormat =
            [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs + 1];]=])
string(FIND "${content}" "${expected}" match)
string(FIND "${content}" "ADD_ATTRIB(NSOpenGLPFAAccelerated);\n    ADD_ATTRIB(NSOpenGLPFAClosestPolicy);" first_attributes)
if(match EQUAL -1 OR first_attributes EQUAL -1)
  message(FATAL_ERROR "Pinned GLFW NSGL source changed; review the software fallback patch")
endif()
string(REPLACE "${expected}" "${replacement}" content "${content}")
file(WRITE "${source}" "${content}")
message(STATUS "Applied documented GLFW macOS software-renderer fallback")
