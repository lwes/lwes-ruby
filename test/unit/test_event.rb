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

  def test_subclass_aset_aref
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    tmp = LWES::Event.subclass "Event1", tdb
    e = tmp.new
    assert_equal({}, e.to_hash)
    vals = {
      :t_bool => true,
      :t_int16 => -1000,
      :t_uint16 => 1000,
      :t_int32 => -64444,
      :t_uint32 => 64444,
      :t_int64 => 10_000_000_000,
      :t_uint64 => 10_000_000_000,
      :t_ip_addr => "192.168.0.1",
      :t_string => "STRING",
      :enc => 0,
      :st => "ruby",
    }
    vals.each do |k,v|
      assert_nothing_raised { e[k.to_s] = v }
      assert_equal v, e[k.to_s], e.to_hash.inspect
    end

    e2 = tmp.new
    vals.each do |k,v|
      assert_nothing_raised { e2[k] = v }
      assert_equal v, e2[k], e2.to_hash.inspect
    end
    assert_equal e2.to_hash, e.to_hash
    e3 = tmp.new
    vals.each do |k,v|
      assert_nothing_raised { e3.__send__ "#{k}=", v }
      assert_equal v, e3.__send__(k), e3.to_hash.inspect
    end
    assert_equal e3.to_hash, e.to_hash
  end

  def test_merge
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    tmp = LWES::Event.subclass "Event1", tdb
    e = tmp.new.merge :t_string => "merged"
    assert_equal "merged", e.t_string
  end

  def test_init_copy
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    tmp = LWES::Event.subclass "Event1", tdb
    a = tmp.new
    b = a.dup
    assert_equal a.to_hash, b.to_hash
    a.t_string = "HELLO"
    assert_equal "HELLO", a.t_string
    assert_nil b.t_string
    c = a.dup
    assert_equal "HELLO", c.t_string
  end

  def test_emit_receive_subclassed
    receiver = UDPSocket.new
    receiver.bind(nil, @options[:port])
    emitter = LWES::Emitter.new(@options)
    tmp = { :t_string => 'hello' }

    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    ev1 = LWES::Event.subclass "Event1", tdb
    emitter.emit "Event1", tmp
    buf, _ = receiver.recvfrom(65536)
    parsed = LWES::Event.parse(buf)
    assert_instance_of ev1, parsed
    assert_equal parsed.to_hash, ev1.new(tmp).to_hash
    ensure
      receiver.close
  end

  def teardown
    LWES::Event::CLASSES.clear
  end
end
