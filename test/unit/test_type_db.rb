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

end
