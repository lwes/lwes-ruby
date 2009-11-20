require "#{File.dirname(__FILE__)}/../test_helper"

class TestStruct < Test::Unit::TestCase

   EXPECT_DB = {
    :SenderPort => LWES::U_INT_16,
    :st => LWES::STRING,
    :enc => LWES::INT_16,
    :SiteID => LWES::U_INT_16,
    :ReceiptTime => LWES::INT_64,
    :SenderIP => LWES::IP_ADDR,

    :t_ip_addr => LWES::IP_ADDR,
    :t_bool => LWES::BOOLEAN,
    :t_uint64 => LWES::U_INT_64,
    :t_uint32 => LWES::U_INT_32,
    :t_int64 => LWES::INT_64,
    :t_string => LWES::STRING,
    :t_int32 => LWES::INT_32,
    :t_uint16 => LWES::U_INT_16,
    :t_int16 => LWES::INT_16
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

  def test_eval
    a = LWES::Struct.new(ESF_FILE, :Eval, :event_class => :Event1) do
      def aaaaaaaa
        true
      end

      class << self
        def get_type_db
          const_get(:TYPE_DB)
        end
      end
    end
    y = a.new
    assert y.respond_to?(:aaaaaaaa)
    assert_instance_of TrueClass, y.aaaaaaaa
    assert_equal a.const_get(:TYPE_DB).object_id, a.get_type_db.object_id
    assert_equal EXPECT_DB, a.get_type_db
  end

end
