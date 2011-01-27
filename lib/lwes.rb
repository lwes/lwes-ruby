module LWES
  # version of our library, currently 0.7.0
  VERSION = "0.7.0"

  autoload :TypeDB, "lwes/type_db"
  autoload :Struct, "lwes/struct"
  autoload :Emitter, "lwes/emitter"
  autoload :Event, "lwes/event"
end
require "lwes_ext"
