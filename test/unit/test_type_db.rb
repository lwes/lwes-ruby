require "#{File.dirname(__FILE__)}/../test_helper"
require 'tempfile'

module Abcde
end

class TestTypeDB < Test::Unit::TestCase

  def test_constants
    assert_instance_of Module, LWES, "LWES is not a module"
    assert_instance_of Class, LWES::TypeDB, "LWES::TypeDB is not a class"
  end

  def test_initialize
    assert_nothing_raised do
      LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf")
    end
  end

  def TODO_test_raises_on_parse_error
    # tmp_err = $stderr.dup
    # $stderr.reopen("/dev/null", "a")
    begin
      LWES::TypeDB.new(__FILE__)
    rescue => e
      pp [:ER, e ]
    end
    # ensure
      # $stderr.reopen(tmp_err)
  end

  def test_to_hash
    expect = {
      :MetaEventInfo => [
        [ :SenderPort, LWES::U_INT_16],
        [ :st, LWES::STRING],
        [ :enc, LWES::INT_16],
        [ :SiteID, LWES::U_INT_16],
        [ :ReceiptTime, LWES::INT_64],
        [ :SenderIP, LWES::IP_ADDR ]
      ],
      :Event1 => [
        [ :t_ip_addr, LWES::IP_ADDR],
        [ :t_bool, LWES::BOOLEAN],
        [ :t_uint64, LWES::U_INT_64],
        [ :t_uint32, LWES::U_INT_32],
        [ :t_int64, LWES::INT_64],
        [ :t_string, LWES::STRING],
        [ :t_int32, LWES::INT_32],
        [ :t_uint16, LWES::U_INT_16],
        [ :t_int16, LWES::INT_16 ]
      ]
    }
    result = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf").to_hash
    assert_equal expect, result
  end

  def test_create_classes
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test2.esf")
    classes = tdb.create_classes!(:parent => TestTypeDB)
    classes.each { |k| assert_kind_of Class, k }
  end

  def test_namespaced_esf
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/namespaced.esf")
    classes = tdb.create_classes!(:parent => Abcde)
    expect = %w(Abcde::A::B::C::Event
                Abcde::A::B::C::Event::Bool
                Abcde::A::B::C::Event::String)
    assert_equal expect, classes.map { |i| i.to_s }
  end

end
