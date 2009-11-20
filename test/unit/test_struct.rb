require "#{File.dirname(__FILE__)}/../test_helper"

class TestStruct < Test::Unit::TestCase

   EXPECT_DB = {
    :SenderPort => :uint16,
    :st => :string,
    :enc => :int16,
    :SiteID => :uint16,
    :ReceiptTime => :int64,
    :SenderIP => :ip_addr,

    :t_ip_addr => :ip_addr,
    :t_bool => :boolean,
    :t_uint64 => :uint64,
    :t_uint32 => :uint32,
    :t_int64 => :int64,
    :t_string => :string,
    :t_int32 => :int32,
    :t_uint16 => :uint16,
    :t_int16 => :int16
  }

  ESF_FILE = "#{File.dirname(__FILE__)}/test1.esf"

  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
  end

  def test_new
    a = LWES::Struct.new(ESF_FILE, :Event1)
    assert a.instance_of?(Class)
    assert_equal "Event1", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    y = a.new
    assert_equal "Event1", y.class.name
    assert_kind_of(::Struct, y)
  end

  def test_new_with_alt_name
    a = LWES::Struct.new(ESF_FILE, :Event, :event_class => :Event1)
    assert a.instance_of?(Class)
    assert_equal "Event", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    y = a.new
    assert_equal "Event", y.class.name
    assert_kind_of(::Struct, y)
  end

  def test_new_in_module
    a = LWES::Struct.new(ESF_FILE, :Event1, :parent_class => LWES)
    assert a.instance_of?(Class)
    assert_equal "LWES::Event1", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    y = a.new
    assert_equal "LWES::Event1", y.class.name
    assert_kind_of(::Struct, y)
  end

end
