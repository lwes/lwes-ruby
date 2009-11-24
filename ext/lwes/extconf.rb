require 'mkmf'
dir_config('lwes')

have_library('lwes') or abort "LWES library not found"
have_header('lwes.h') or abort "lwes.h not found"
have_func('memrchr', 'string.h')
create_makefile('lwes_ext')
