module LWES
  # version of our library, currently 0.2.2
  VERSION = "0.2.2"

  autoload :TypeDB, "lwes/type_db"
  autoload :Struct, "lwes/struct"
  autoload :Emitter, "lwes/emitter"
end
require "lwes_ext"
