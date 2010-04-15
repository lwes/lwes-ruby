require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"
require 'ipaddr'

class TestEmitter < Test::Unit::TestCase
  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
    assert_instance_of Class, LWES::Emitter, "LWES::Emitter is not a class"
  end

  def setup
    @options = LISTENER_DEFAULTS.dup
  end

  def test_nil_iface
    @options.delete(:iface)
    assert_instance_of LWES::Emitter, LWES::Emitter.new(@options)
  end

  def test_initialize
    assert_instance_of LWES::Emitter, LWES::Emitter.new(@options)
  end

  def test_dup
    orig = LWES::Emitter.new(@options)
    duped = orig.dup
    assert_instance_of LWES::Emitter, duped
    assert duped.object_id != orig.object_id
  end

  def test_clone
    orig = LWES::Emitter.new(@options)
    cloned = orig.clone
    assert_instance_of LWES::Emitter, cloned
    assert cloned.object_id != orig.object_id
  end

  def test_initialize_with_heartbeat
    heartbeat = @options.merge(:heartbeat => 30)
    assert_instance_of LWES::Emitter, LWES::Emitter.new(heartbeat)
  end

  def test_initialize_no_ttl
    no_ttl = @options.dup
    no_ttl.delete(:ttl)
    assert_instance_of LWES::Emitter, LWES::Emitter.new(no_ttl)
  end

  def test_initialize_invalid
    assert_raises(TypeError) {
      LWES::Emitter.new(@options.merge(:address => nil))
    }
  end

  def test_initialize_empty_options
    assert_raises(TypeError) { LWES::Emitter.new({}) }
  end

  def test_emit_invalid
    emitter = LWES::Emitter.new(@options)
    assert_raises(TypeError) { emitter.emit "Invalid", nil }
    assert_raises(ArgumentError) { emitter.emit nil, { :hello => "world" }}
  end

  def test_emit_empty_hash
    emitter = LWES::Emitter.new(@options)
    assert_nothing_raised { emitter.emit("Valid", Hash.new) }
  end

  def test_emit_non_empty_hashes
    emitter = LWES::Emitter.new(@options)
    out = lwes_listener do
      assert_nothing_raised {
        emitter.emit("ASDF", { :foo => "FOO", :nr => [ :int16, 50 ] })
      }
    end
    lines = out.readlines
    assert_match %r{\AASDF\b}, lines.first
    assert ! lines.grep(/foo = FOO;/).empty?
    assert ! lines.grep(/nr = 50;/).empty?
  end

  def test_emit_ip_addr_string
    emitter = LWES::Emitter.new(@options)
    event = { :string_ip => [ :ip_addr, "192.168.1.1" ] }
    out = lwes_listener do
      assert_nothing_raised { emitter.emit("STRING_IP", event) }
    end
    lines = out.readlines
    assert_equal 1, lines.grep(/string_ip = 192.168.1.1/).size
  end

  def test_emit_ip_addr_int
    emitter = LWES::Emitter.new(@options)
    event = { :int_ip => [ :ip_addr, IPAddr.new("192.168.1.1").to_i ] }
    out = lwes_listener do
      assert_nothing_raised { emitter.emit("INT_IP", event) }
    end
    lines = out.readlines
    assert_equal 1, lines.grep(/int_ip = 192.168.1.1/).size
  end

  def TODO_emit_ip_addr_object
    emitter = LWES::Emitter.new(@options)
    event = { :ip => IPAddr.new("192.168.1.1") }
    out = lwes_listener do
      assert_nothing_raised { emitter.emit("IP", event) }
    end
    lines = out.readlines
    assert_equal 1, lines.grep(/\bip = 192.168.1.1/).size
  end

  def test_emit_invalid
    emitter = LWES::Emitter.new(@options)
    assert_raises(ArgumentError) { emitter.emit("JUNK", :junk => %r{junk}) }
  end

  def test_emit_booleans
    emitter = LWES::Emitter.new(@options)
    event = { :true => true, :false => false }
    out = lwes_listener do
      assert_nothing_raised { emitter.emit("BOOLS", event)
      }
    end
    lines = out.readlines
    assert_equal 1, lines.grep(/true = true;/).size
    assert_equal 1, lines.grep(/false = false;/).size
  end

  def test_emit_numeric_ranges
    check_min_max(:int16, -0x7fff - 1, 0x7fff)
    check_min_max(:int32, -0x7fffffff - 1, 0x7fffffff)
    check_min_max(:int64, -0x7fffffffffffffff - 1, 0x7fffffffffffffff)
    check_min_max(:uint16, 0, 0xffff)
    check_min_max(:uint32, 0, 0xffffffff)
    check_min_max(:uint64, 0, 0xffffffffffffffff)
    huge = 0xffffffffffffffffffffffffffffffff
    tiny = -0xffffffffffffffffffffffffffffffff
    [ :int16, :int32, :int64, :uint16, :uint32, :uint64 ].each do |type|
      emitter = LWES::Emitter.new(@options)
      assert_raises(RangeError) {
        emitter.emit("way over", { type => [ type, huge ] })
      }
      assert_raises(RangeError) {
        emitter.emit("way under", { type => [ type, tiny ] })
      }
    end
  end

  def check_min_max(type, min, max)
    emitter = LWES::Emitter.new(@options)
    out = lwes_listener do
      assert_raises(RangeError) {
        emitter.emit("over", { type => [ type, max + 1] })
      }
      assert_raises(RangeError, "type=#{type} min=#{min}") {
        emitter.emit("under", { type => [ type, min - 1 ] })
      }
      assert_nothing_raised {
        emitter.emit("zero", { type => [ type, 0 ] })
      }
      assert_nothing_raised {
        emitter.emit("min", { type => [ type, min ] })
      }
      assert_nothing_raised {
        emitter.emit("max", { type => [ type, max ] })
      }
    end
    lines = out.readlines
    assert lines.grep(/\Aover/).empty?
    assert lines.grep(/\Aunder/).empty?
    assert_equal 1, lines.grep(/\Amax/).size
    assert_equal 1, lines.grep(/\Amin/).size
    assert_equal 1, lines.grep(/\Azero/).size
  end

  def test_emit_uint64
    emitter = LWES::Emitter.new(@options)
    assert_nothing_raised {
      emitter.emit("Foo", { :uint64 => [ :uint64, 10_000_000_000 ] })
    }
  end

  def test_close
    emitter = LWES::Emitter.new(@options)
    assert_nil emitter.close
  end

end
