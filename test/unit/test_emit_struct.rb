require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"

class InvalidStruct1 < Struct.new(:invalid)
  TYPE_DB = []
end

class TestEmitStruct < Test::Unit::TestCase

  def setup
    assert_kind_of Class, self.class.const_get(:Event1)
    # assert self.class.const_get(:Event1).kind_of?(Struct)
    @options = LISTENER_DEFAULTS.dup
  end

  def test_emit_non_lwes_struct
    emitter = LWES::Emitter.new(@options)
    assert_raise(ArgumentError) { emitter << InvalidStruct1.new }
  end

  def test_emit_crap
    emitter = LWES::Emitter.new(@options)
    assert_raise(TypeError) { emitter << "HHI" }
    assert_raise(TypeError) { emitter << [] }
    assert_raise(TypeError) { emitter << {} }
  end

  def test_emit_struct_full
    s = nil
    out = lwes_listener do
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
    out = out.readlines
    s.members.each do |m|
      value = s[m.to_sym] or next
      regex = /\b#{m} = #{value};/
      assert_equal 1, out.grep(regex).size,
                   "#{regex.inspect} didn't match #{out.inspect}"
    end
  end

  def test_emit_struct_coerce_type
    s = nil
    out = lwes_listener do
      assert_nothing_raised do
        emitter = LWES::Emitter.new(@options)
        s = Event1.new
        s.t_bool = true
        s.t_int16 = '-1000'
        s.t_uint16 = 1000
        s.t_int32 = -64444
        s.t_uint32 = 64444
        s.t_int64 = 10_000_000_000
        s.t_uint64 = 10_000_000_000
        s.t_ip_addr = '192.168.0.1'
        s.t_string = 1234567
        emitter.emit(s)
      end
    end
    out = out.readlines
    s.members.each do |m|
      value = s[m.to_sym] or next
      regex = /\b#{m} = #{value};/
      assert_equal 1, out.grep(regex).size,
                   "#{regex.inspect} didn't match #{out.inspect}"
    end
  end

  def test_emit_struct_cplusplus
    s = nil
    out = lwes_listener do
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
        emitter << s
      end
    end
    out = out.readlines
    s.members.each do |m|
      value = s[m.to_sym] or next
      regex = /\b#{m} = #{value};/
      assert_equal 1, out.grep(regex).size,
                   "#{regex.inspect} didn't match #{out.inspect}"
    end
  end

  def test_emit_from_class
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
    out = lwes_listener do
      assert_nothing_raised do
        emitter = LWES::Emitter.new(@options)
        emitter.emit(Event1, opt)
      end
    end
    out = out.readlines
    opt.each do |m, value|
      regex = /\b#{m} = #{value};/
      assert_equal 1, out.grep(regex).size,
                   "#{regex.inspect} didn't match #{out.inspect}"
    end
  end

  def test_emit_from_class_bad_type
    out = lwes_listener do
      assert_raises(TypeError) do
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
    assert out.readlines.empty?
  end

  def test_emit_alt_class_name
    out = lwes_listener do
      emitter = LWES::Emitter.new(@options)
      emitter.emit(Event, :t_uint32 => 16384)
    end
    out = out.readlines
    assert_match %r{^Event1\[\d+\]}, out.first
  end

end

ESF_FILE = "#{File.dirname(__FILE__)}/test1.esf"
LWES::Struct.new(:file=>ESF_FILE,
                 :parent => TestEmitStruct)
LWES::Struct.new(:file=>ESF_FILE,
                 :parent => TestEmitStruct,
                 :name => "Event1",
                 :class => "Event")
