require "#{File.dirname(__FILE__)}/../test_helper"

class TestEmitStruct < Test::Unit::TestCase

  def setup
    assert_kind_of Class, self.class.const_get(:Event1)
    # assert self.class.const_get(:Event1).kind_of?(Struct)
    @options = {
      :address => ENV["LWES_TEST_ADDRESS"] || "127.0.0.1",
      :iface => ENV["LWES_TEST_IFACE"] || "0.0.0.0",
      :port => ENV["LWES_TEST_PORT"] ? ENV["LWES_TEST_PORT"].to_i : 12345,
      :ttl => 60, # nil for no ttl)
    }
  end

  def test_emit_struct_full
    assert_nothing_raised do
      emitter = LWES::Emitter.new(@options)
      s = Event1.new
      s.t_bool = true
      s.t_int16 = -1000
      s.t_uint16 = 1000
      s.t_int32 = -64444
      s.t_uint32 = 64444
      s.t_int64 = 10_000_000_000
      s.t_uint64 = 10_000_000_000
      s.t_ip_addr = '192.168.0.1'
      s.t_string = "STRING"
      emitter.emit(s)
    end
  end

  def test_emit_from_class
    assert_nothing_raised do
      emitter = LWES::Emitter.new(@options)
      opt = {
        :t_bool => true,
        :t_int16 => -1000,
        :t_uint16 => 1000,
        :t_int32 => -64444,
        :t_uint32 => 64444,
        :t_int64 => 10_000_000_000,
        :t_uint64 => 10_000_000_000,
        :t_ip_addr => '192.168.0.1',
        :t_string => "STRING",
      }
      emitter.emit(Event1, opt)
    end
  end

  def test_emit_from_class_bad_type
    e = assert_raises(TypeError) do
      emitter = LWES::Emitter.new(@options)
      opt = {
        :t_int16 => -1000,
        :t_uint16 => 1000,
        :t_int32 => -64444,
        :t_uint32 => 64444,
        :t_int64 => 10_000_000_000,
        :t_uint64 => 10_000_000_000,
        :t_ip_addr => '192.168.0.1',
        :t_string => true, #"STRING",
      }
      emitter.emit(Event1, opt)
    end
  end

end

ESF_FILE = "#{File.dirname(__FILE__)}/test1.esf"
LWES::Struct.new(:file=>ESF_FILE,
                 :parent => TestEmitStruct)
