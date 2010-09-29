require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"
require 'ipaddr'

class TestEvent < Test::Unit::TestCase

  def setup
    @options = LISTENER_DEFAULTS.dup
  end

  def test_inspect
    ev = LWES::Event.new
    assert_instance_of String, ev.inspect
  end

  def test_to_hash
    ev = LWES::Event.new
    assert_equal({}, ev.to_hash)
  end

  def test_emit_recieve_hash
    receiver = UDPSocket.new
    receiver.bind(nil, @options[:port])
    emitter = LWES::Emitter.new(@options)
    tmp = {
      :str => 'hello',
      :uint16 => [ :uint16, 6344 ],
      :int16 => [ :int16, -6344 ],
      :uint32 => [ :uint32, 6344445 ],
      :int32 => [ :int32, -6344445 ],
      :uint64 => [ :uint64, 6344445123123 ],
      :int64 => [ :int64, -6344445123123 ],
      :true => true,
      :false => false,
      :addr => [ :ip_addr, "127.0.0.1" ],
    }

    emitter.emit "Event", tmp
    buf, addr = receiver.recvfrom(65536)
    parsed = LWES::Event.parse(buf)
    expect = {
      :name => "Event",
      :uint16 => 6344,
      :str => "hello",
      :int16 => -6344,
      :uint32 => 6344445,
      :int32 => -6344445,
      :uint64 => 6344445123123,
      :int64 => -6344445123123,
      :true => true,
      :false => false,
      :addr => "127.0.0.1",
    }
    assert_instance_of LWES::Event, parsed
    assert_equal expect, parsed.to_hash

    # test for round tripping
    emitter.emit parsed
    buf, addr = receiver.recvfrom(65536)
    assert_instance_of LWES::Event, parsed
    assert_equal expect, parsed.to_hash

    emitter << parsed
    buf, addr = receiver.recvfrom(65536)
    assert_instance_of LWES::Event, parsed
    assert_equal expect, parsed.to_hash
    ensure
      receiver.close
  end
end
