module LWES
  # version of our library, currently 0.6.1
  VERSION = "0.6.1"

  autoload :TypeDB, "lwes/type_db"
  autoload :Struct, "lwes/struct"
  autoload :Emitter, "lwes/emitter"
  autoload :Event, "lwes/event"
end
require "lwes_ext"
