require 'lwes'
#
# This file contains convenience classes that wrap lwes functions.
#
# Author:: Frank Maritato

# This class wraps an lwes listener.
# Example usage:
# event = Event.new("testeventtypedb.esf", nil);
# listener = Listener.new("224.1.1.11", "0.0.0.0", 12345);
# while (true)
#   rtn = listener.receive(event);
#   if (rtn > 0)
#     print "Received event: #{event}\n";
#   end
# end
class Listener

  def initialize(multicast_listen_address, listen_interface, multicast_listen_port)
    @listener = Lwes.lwes_listener_create(multicast_listen_address,
                                          listen_interface,
                                          multicast_listen_port);
  end

  def receive(event)
    return Lwes.lwes_listener_recv(@listener, event.get_event);
  end

  def get_listener
    return @listener;
  end
end

# This class wraps an lwes emitter.
# Example usage:
# event = Event.new("testeventtypedb.esf", "Testing");
# event.set_str("aString", "garbage");
# event.set_bool("aBoolean", 1);
# event.set_uint16("aUInt16", 2);
# event.set_int16("anInt16", 3);
# emitter = Emitter.new("224.1.1.11", "0.0.0.0", 12345, 1, 1);
# emitter.emit(event);
class Emitter

  def initialize(
                 multicast_listen_address,
                 listen_interface,
                 multicast_listen_port,
                 emit_heartbeat,
                 heartbeat_frequency
                )

    @emitter = Lwes.lwes_emitter_create(multicast_listen_address,
                                        listen_interface,
                                        multicast_listen_port,
                                        emit_heartbeat,
                                        heartbeat_frequency);
  end

  def emit(event)
    Lwes.lwes_emitter_emit(@emitter, event.get_event);
  end

  def get_emitter
    return @emitter;
  end
end

# This class wraps an lwes event. You will probably want to subclass this
# as convenience functions for your application's specific events.
# Example usage:
# event = Event.new("testeventtypedb.esf", "Testing");
# event.set_str("aString", "garbage");
# event.set_bool("aBoolean", 1);
# event.set_uint16("aUInt16", 2);
# event.set_int16("anInt16", 3);
class Event

  def initialize(file, name)
    @event_db = Lwes.lwes_event_type_db_create(file);
    if (name == nil)
      @event = Lwes.lwes_event_create_no_name(@event_db);
    else
      @event = Lwes.lwes_event_create(@event_db, name);
    end
  end

  def set_str(name, value)
    Lwes.lwes_event_set_STRING(@event, name, value);
  end
  def set_bool(name, value)
    Lwes.lwes_event_set_BOOLEAN(@event, name, value);
  end
  def set_uint16(name, value)
    Lwes.lwes_event_set_U_INT_16(@event, name, value);
  end
  def set_uint32(name, value)
    Lwes.lwes_event_set_U_INT_32(@event, name, value);
  end
  def set_uint64(name, value)
    Lwes.lwes_event_set_U_INT_64(@event, name, value);
  end
  def set_int16(name, value)
    Lwes.lwes_event_set_INT_16(@event, name, value);
  end
  def set_int32(name, value)
    Lwes.lwes_event_set_INT_32(@event, name, value);
  end
  def set_int64(name, value)
    Lwes.lwes_event_set_INT_64(@event, name, value);
  end
  def get_event
    return @event
  end
end
