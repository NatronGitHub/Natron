#!/bin/bash
# Generate the glad.h and glad.c file using glad installed via pip, see:
# https://github.com/Dav1dde/glad

CWD=`pwd`
echo "This script should be called from the root directory (Natron)"

#"GL_ARB_texture_non_power_of_two " // or GL_IMG_texture_npot, or GL_OES_texture_npot, core since 2.0
#"GL_ARB_shader_objects " // GLSL, Uniform*, core since 2.0


GL_EXTENSIONS="GL_ARB_vertex_buffer_object,GL_ARB_pixel_buffer_object,GL_ARB_vertex_array_object,GL_ARB_framebuffer_object,GL_ARB_texture_float,GL_EXT_framebuffer_object,GL_APPLE_vertex_array_object"

python -m glad --profile=compatibility --api="gl=2.0" --generator=c-debug --spec=gl --extensions=$GL_EXTENSIONS --omit-khrplatform --out-path=$CWD/Global/gladDeb

python -m glad --profile=compatibility --api="gl=2.0" --generator=c --spec=gl --extensions=$GL_EXTENSIONS --omit-khrplatform --out-path=$CWD/Global/gladRel