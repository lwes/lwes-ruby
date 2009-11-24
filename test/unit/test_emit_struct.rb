require "#{File.dirname(__FILE__)}/../test_helper"

class TestEmitStruct < Test::Unit::TestCase

  ESF_FILE = "#{File.dirname(__FILE__)}/test1.esf"

  def setup
    @options = {
      :address => ENV["LWES_TEST_ADDRESS"] || "127.0.0.1",
      :iface => ENV["LWES_TEST_IFACE"] || "0.0.0.0",
      :port => ENV["LWES_TEST_PORT"] ? ENV["LWES_TEST_PORT"].to_i : 12345,
      :ttl => 60, # nil for no ttl)
    }
  end

  def test_emit_struct_full
    emitter = LWES::Emitter.new(@options)
    a = LWES::Struct.new(:file=>ESF_FILE,
                     :class=> :TestEmitStructFull,
                     :name => :Event1,
                     :optional=> %r{\A([SR]|enc|st)})
    s = ::TestEmitStructFull.new
    s.t_bool = true
    s.t_int16 = -1000
    s.t_uint16 = 1000
    s.t_int32 = -64444
    s.t_uint32 = 64444
    s.t_int64 = 10_000_000_000
    s.t_uint64 = 10_000_000_000
    s.t_ip_addr = '192.168.0.1'
    s.t_string = "STRING"
    emitter.emit('Event1', s)
  end

end
