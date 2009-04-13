require 'lwes_functions'

event = Event.new("sample.esf", "Testing");
event.set_str("aString", "garbage");
event.set_bool("aBoolean", 1);
event.set_uint16("aUInt16", 2);
event.set_int16("anInt16", 3);
emitter = Emitter.new("224.1.1.11", "0.0.0.0", 12345, 1, 1);
emitter.emit(event);
