# recomp_gl.cmake — recomp_resolve_gl(<out_var>): the GL library to link.
# Same contract as recomp-ui's helper: the psxrecomp runtime includes this file
# from RECOMP_UI_ROOT when it exists. GLVND hosts (Steam Deck) have
# OpenGL::OpenGL but no OpenGL::GL, so prefer whichever target exists.
include_guard(GLOBAL)

function(recomp_resolve_gl out_var)
    if(WIN32)
        set(${out_var} opengl32 PARENT_SCOPE)
        return()
    endif()
    find_package(OpenGL REQUIRED)
    foreach(_cand OpenGL::GL OpenGL::OpenGL)
        if(TARGET ${_cand})
            set(${out_var} ${_cand} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    foreach(_lib OPENGL_gl_LIBRARY OPENGL_opengl_LIBRARY)
        if(${_lib})
            set(${out_var} "${${_lib}}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "recomp-launcher: no usable OpenGL library found")
endfunction()
