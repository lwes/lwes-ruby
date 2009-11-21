require "#{File.dirname(__FILE__)}/../test_helper"

module Dummy
end
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
  EXPECT_LIST = [
    # class-specific fields first
    [ :t_bool, LWES::BOOLEAN],
    [ :t_int16, LWES::INT_16 ],
    [ :t_int32, LWES::INT_32],
    [ :t_int64, LWES::INT_64],
    [ :t_ip_addr, LWES::IP_ADDR],
    [ :t_string, LWES::STRING],
    [ :t_uint16, LWES::U_INT_16],
    [ :t_uint32, LWES::U_INT_32],
    [ :t_uint64, LWES::U_INT_64],

    # MetaEventInfo
    [ :ReceiptTime, LWES::INT_64],
    [ :SenderIP, LWES::IP_ADDR],
    [ :SenderPort, LWES::U_INT_16],
    [ :SiteID, LWES::U_INT_16],
    [ :enc, LWES::INT_16],
    [ :st, LWES::STRING],
  ]

  ESF_FILE = "#{File.dirname(__FILE__)}/test1.esf"

  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
  end

  def test_new_with_defaults
    a = LWES::Struct.new(:file=>ESF_FILE)
    assert a.instance_of?(Class)
    assert_equal "Event1", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    assert_equal EXPECT_LIST, a.const_get(:TYPE_LIST)
    y = a.new
    assert_equal "Event1", y.class.name
    assert_kind_of(::Struct, y)
  end

  def test_new_with_alt_name
    a = LWES::Struct.new(:file=>ESF_FILE,:class=>:Event,:name=>:Event1)
    assert a.instance_of?(Class)
    assert_equal "Event", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    y = a.new
    assert_equal "Event", y.class.name
    assert_kind_of(::Struct, y)
  end

  def test_new_in_module
    a = LWES::Struct.new(:file=>ESF_FILE,:class=>:Event1,:parent=>LWES)
    assert a.instance_of?(Class)
    assert_equal "LWES::Event1", a.name
    assert a.const_get(:TYPE_DB).instance_of?(Hash)
    assert_equal EXPECT_DB, a.const_get(:TYPE_DB)
    y = a.new
    assert_equal "LWES::Event1", y.class.name
    assert_kind_of(::Struct, y)
  end

  def test_eval
    a = LWES::Struct.new(:file=>ESF_FILE,:class=>:Eval,:name=>:Event1) do
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

  def test_optional
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=>:Optional,
                         :name=>:Event1,
                         :optional => %w(SenderPort SiteID ReceiptTime))
    expect_optional = {
      :SenderPort => true,
      :SiteID => true,
      :ReceiptTime => true
    }
    assert_equal expect_optional, a.const_get(:OPTIONAL)
  end

  def test_optional_meta_event_info
    expect = {
      :ReceiptTime => true,
      :SenderIP => true,
      :SenderPort => true,
      :SiteID => true,
      :enc => true,
      :st => true
    }
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=>:MetaOptional,
                         :name=>:Event1,
                         :optional => %w(MetaEventInfo))
    assert_equal expect, a.const_get(:OPTIONAL)
  end


  def test_new_initialize
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=>:Init,
                         :name=>:Event1,
                         :parent=>Dummy)
    y = Dummy::Init.new(:t_ip_addr => "192.168.0.1", :t_bool => true)
    assert_equal "192.168.0.1", y.t_ip_addr
    assert_equal true, y.t_bool
  end

  def test_new_initialize_defaults
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_Blah,
                         :name=>:Event1,
                         :defaults => { :enc => 1 })
    y = LWES_Blah.new
    assert_equal 1, y.enc
  end

  def test_new_initialize_defaults_and_hash
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_Foo,
                         :name=>:Event1,
                         :defaults => { :enc => 1 })
    y = LWES_Foo.new(:enc => 2)
    assert_equal 2, y.enc
  end

  def test_new_initialize_struct_style
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_New,
                         :name=>:Event1)
    y = LWES_Foo.new(true, -16, -32, -64, "127.0.0.1", "HI", 16, 32, 64)
    assert_equal true, y.t_bool
    assert_equal -16, y.t_int16
    assert_equal -32, y.t_int32
    assert_equal -64, y.t_int64
    assert_equal "127.0.0.1", y.t_ip_addr
    assert_equal "HI", y.t_string
    assert_equal 16, y.t_uint16
    assert_equal 32, y.t_uint32
    assert_equal 64, y.t_uint64
  end

  def test_skip_attr
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_Skip,
                         :name=>:Event1,
                         :skip => %w(SenderIP SenderPort SiteID ReceiptTime))
    y = LWES_Skip.new
    assert ! y.respond_to?(:SenderIP)
    assert ! y.respond_to?(:SenderPort)
    assert ! y.respond_to?(:SiteID)
    assert ! y.respond_to?(:ReceiptTime)
    assert y.respond_to?(:enc)
    assert y.respond_to?(:st)
  end

  def test_skip_regexp
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_SkipRe,
                         :name=>:Event1,
                         :skip => %r(\AS))
    y = LWES_SkipRe.new
    assert ! y.respond_to?(:SenderIP)
    assert ! y.respond_to?(:SenderPort)
    assert ! y.respond_to?(:SiteID)
    assert y.respond_to?(:ReceiptTime)
    assert y.respond_to?(:enc)
    assert y.respond_to?(:st)
  end

  def test_optional_regexp
    a = LWES::Struct.new(:file=>ESF_FILE,
                         :class=> :LWES_OptionalRe,
                         :name=>:Event1,
                         :optional=> %r(\AS))
    expect = {:SenderIP=>true, :SenderPort=>true, :SiteID=>true}
    assert_equal expect, LWES_OptionalRe::OPTIONAL
  end

end
