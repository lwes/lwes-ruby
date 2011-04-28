##
# = Light Weight Event System bindings for Ruby
#
#    require 'lwes'
#
#    # create an Emitter which may be used for the lifetime of your process
#    emitter = LWES::Emitter.new(:address => '224.1.1.11',
#                                :port => 12345,
#                                :heartbeat => 30, # nil to disable
#                                :ttl => 1) # nil for default TTL(3)
#
#    # parse your ESF file at startup, the example below assumes you
#    # have "Event1" and "Event2" defined in your ESF file:
#    type_db = LWES::TypeDB.new("my_events.esf")
#
#    # create classes to use, by default and to encourage DRY-ness,
#    # we map each event in the ESF file to a class
#    # Optionally, you may specify :parent => module/namespace
#    type_db.create_classes! :parent => MyApp
#
#    # inside your application, you may now do this to slowly build up
#    # fields for the event
#    my_event = MyApp::Event1.new
#    my_event.started = Time.now.to_i
#    my_event.remote_addr = "192.168.0.1"
#    # ...do a lot of stuff here in between...
#    my_event.field1 = value1
#    my_event.field2 = value2
#    my_event.field3 = value3
#    my_event.finished = Time.now.to_i
#    emitter << my_event
#
#    # Alternatively, if you know ahead of time all the fields you want to
#    # set for an event, you can emit an event in one step:
#
#    emitter.emit MyApp::Event2, :field1 => value1, :field2 => value2 # ...
#
module LWES
  # version of our library, currently 0.8.1
  VERSION = "0.8.1"

  autoload :ClassMaker, "lwes/class_maker"
  autoload :TypeDB, "lwes/type_db"
  autoload :Struct, "lwes/struct"
  autoload :Emitter, "lwes/emitter"
  autoload :Event, "lwes/event"
  autoload :Listener, "lwes/listener"
end
require "lwes_ext"
