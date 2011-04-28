# The LWES::Emitter is used for emitting LWES events to a multicast
# network or a single host.  It can emit LWES::Event objects, LWES::Struct
# objects, and even plain Ruby hashes.
#
# It is non-blocking and does not guarantee delivery.
#
#
#    emitter = LWES::Emitter.new(:address => '224.1.1.11',
#                                :port => 12345,
#                                :heartbeat => 30, # nil to disable
#                                :ttl => 1) # nil for default TTL(3)
#    event = MyEvent.new
#    event.foo = "bar"
#
#    emitter << event
#
# === NON-ESF USERS
#
# Since we can't reliably map certain Ruby types to LWES types, you'll
# have to specify them explicitly for IP addresses and all Integer
# types.
#
#    event = {
#      :time_sec => [ :int32, Time.now.to_i ],
#      :time_usec => [ :int32, Time.now.tv_usec ],
#      :remote_addr => [ :ip_addr, "192.168.0.1" ],
#    }
#
#    # Strings and Boolean values are easily mapped, however:
#    event[:field1] = "String value"
#    event[:boolean1] = true
#    event[:boolean2] = false
#
#    # finally, we just emit the hash with any given name
#    emitter.emit "Event3", event
class LWES::Emitter

  # creates a new Emitter object which may be used for the lifetime
  # of the process:
  #
  #   LWES::Emitter.new(:address => '224.1.1.11',
  #                     :iface => '0.0.0.0',
  #                     :port => 12345,
  #                     :heartbeat => false, # Integer for frequency
  #                     :ttl => 60, # nil for no ttl)
  #
  def initialize(options = {}, &block)
    options[:iface] ||= '0.0.0.0'
    _create(options)
    block_given?
  end
end
