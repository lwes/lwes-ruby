require 'lwes_functions'

event = Event.new("sample.esf", nil);
listener = Listener.new("224.1.1.11", "0.0.0.0", 12345);
while (true)
  rtn = listener.receive(event);
  if (rtn > 0)
    print "Received event: #{event}\n";
  end
end
