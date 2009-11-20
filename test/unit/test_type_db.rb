require "#{File.dirname(__FILE__)}/../test_helper"
require 'tempfile'

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
      :MetaEventInfo => {
        :SenderPort => :uint16,
        :st => :string,
        :enc => :int16,
        :SiteID => :uint16,
        :ReceiptTime => :int64,
        :SenderIP => :ip_addr
      },
      :Event1 => {
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
    }
    result = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test1.esf").to_hash
    assert_equal expect, result
  end

end
