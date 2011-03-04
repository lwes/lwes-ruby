require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"

class TestListener < Test::Unit::TestCase
  def setup
    @options = LISTENER_DEFAULTS.dup
    @listener = @emitter = nil
  end

  def teardown
    assert_nil(@listener.close) if @listener
    assert_nil(@emitter.close) if @emitter
    LWES::Event::CLASSES.clear
  end

  def test_listen_and_close
    listener = LWES::Listener.new @options.merge(:iface => nil)
    assert_nil listener.close
    assert_raises(IOError) { listener.close }
  end

  def test_listen_and_close_with_iface
    listener = LWES::Listener.new @options
    assert_nil listener.close
    assert_raises(IOError) { listener.close }
  end

  def test_listen_emit_recv_and_close
    @listener = LWES::Listener.new @options
    @emitter = LWES::Emitter.new @options
    @emitter.emit("E1", { :hello => "WORLD"})
    event = @listener.recv.to_hash
    assert_equal "E1", event[:name]
    assert event[:SenderPort] > 0
    assert event[:ReceiptTime] > (Time.now.to_i * 1000)
    assert_equal "WORLD", event[:hello]
    assert_equal @options[:address], event[:SenderIP]
  end

  def test_listen_emit_custom_event_recv_and_close
    @listener = LWES::Listener.new @options
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    tmp = LWES::Event.subclass :name => "Event1", :db => tdb, :parent => nil
    @emitter = LWES::Emitter.new @options
    @emitter << tmp.new(:t_string => "HI")
    event = @listener.recv
    assert_instance_of tmp, event
    assert_equal "HI", event.t_string
    assert event.SenderPort > 0
    assert event.ReceiptTime > (Time.now.to_i * 1000)
    assert_equal @options[:address], event.SenderIP
  end

  def test_listen_recv_timeout
    @listener = LWES::Listener.new @options
    t0 = Time.now.to_f
    event = @listener.recv 10
    assert_nil event
    delta = Time.now.to_f - t0
    assert(delta >= 0.01, "delta=#{delta}")
  end

  def test_listen_each_signal
    signaled = false
    handler = trap(:USR1) { signaled = true }
    @listener = LWES::Listener.new @options
    @emitter = LWES::Emitter.new @options
    tmp = []
    thr = Thread.new do
      sleep 0.1
      Process.kill :USR1, $$
      @emitter.emit("E1", :hello => "WORLD")
      :OK
    end
    @listener.each { |event| tmp << event and break }
    assert thr.join
    assert_equal :OK, thr.value
    assert_equal 1, tmp.size
    assert_equal "WORLD", tmp[0].to_hash[:hello]
    ensure
      trap(:USR1, handler)
  end
end if LWES::Listener.method_defined?(:recv)
