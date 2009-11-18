require "#{File.dirname(__FILE__)}/../test_helper"

class TestEmitter < Test::Unit::TestCase

  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
    assert_instance_of Class, LWES::Emitter, "LWES::Emitter is not a class"
  end

  def setup
    @options = {
      :address => ENV["LWES_ADDRESS"] || "127.0.0.1",
      :iface => ENV["LWES_IFACE"] || "0.0.0.0",
      :port => ENV["LWES_PORT"] ? ENV["LWES_PORT"].to_i : 12345,
      :ttl => 60, # nil for no ttl)
    }
  end

  def test_initialize
    assert_instance_of LWES::Emitter, LWES::Emitter.new(@options)
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
  end

  def test_emit_empty_hash
    emitter = LWES::Emitter.new(@options)
    assert_nothing_raised { emitter.emit("Valid", Hash.new) }
  end

  def test_emit_nameless_hashes
    emitter = LWES::Emitter.new(@options)
    assert_nothing_raised {
      emitter.emit("Nameless", { :foo => "FOO", :nr => [ :int16, 50 ] })
    }
  end

  def test_close
    emitter = LWES::Emitter.new(@options)
    assert_nil emitter.close
  end

end
