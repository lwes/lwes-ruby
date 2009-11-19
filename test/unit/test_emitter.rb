require "#{File.dirname(__FILE__)}/../test_helper"
require 'tempfile'

class TestEmitter < Test::Unit::TestCase
  BEFORE_DELAY = ENV['BEFORE_DELAY'] ? ENV['BEFORE_DELAY'].to_f : 0.5
  AFTER_DELAY = ENV['AFTER_DELAY'] ? ENV['AFTER_DELAY'].to_f : 0.5

  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
    assert_instance_of Class, LWES::Emitter, "LWES::Emitter is not a class"
  end

  def lwes_listener(&block)
    cmd = "lwes-event-printing-listener" \
          " -m #{@options[:address]}" \
          " -i #{@options[:iface]}" \
          " -p #{@options[:port]}"
    out = Tempfile.new("out")
    err = Tempfile.new("err")
    $stdout.flush
    $stderr.flush
    pid = fork do
      $stdout.reopen(out.path)
      $stderr.reopen(err.path)
      exec cmd
    end
    begin
      # since everything executes asynchronously and our messaging,
      # we need to ensure our listener is ready, then ensure our
      # listener has printed something...
      # XXX racy
      sleep BEFORE_DELAY
      yield
      sleep AFTER_DELAY
    ensure
      Process.kill(:TERM, pid)
      Process.waitpid2(pid)
      assert_equal 0, err.size
    end
    out
  end

  def setup
    @options = {
      :address => ENV["LWES_TEST_ADDRESS"] || "127.0.0.1",
      :iface => ENV["LWES_TEST_IFACE"] || "0.0.0.0",
      :port => ENV["LWES_TEST_PORT"] ? ENV["LWES_TEST_PORT"].to_i : 12345,
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

  def test_close
    emitter = LWES::Emitter.new(@options)
    assert_nil emitter.close
  end

end
