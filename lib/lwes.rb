module LWES
  # version of our library, currently 0.3.0
  VERSION = "0.3.0"

  autoload :TypeDB, "lwes/type_db"
  autoload :Struct, "lwes/struct"
  autoload :Emitter, "lwes/emitter"
end
require "lwes_ext"
