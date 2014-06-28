require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"
require 'tempfile'

module Abcde;
end

class TestTypeDBEvents < Test::Unit::TestCase
  def test_create_classes
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/test2.esf")
    tmp = Module.new
    classes = tdb.create_classes!(:parent => tmp, :sparse => true)
    classes.each { |k| assert_equal LWES::Event, k.superclass }
  end

  def test_namespaced_esf
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/namespaced.esf")
    classes = tdb.create_classes!(:parent => Abcde, :sparse => true)
    expect = %w(Abcde::A::B::C::Event
                Abcde::A::B::C::Event::Bool
                Abcde::A::B::C::Event::String)
    assert_equal expect, classes.map { |i| i.to_s }
    Abcde.__send__ :remove_const, :A
    # LWES::Event::CLASSES.clear
  end

  def test_empty_struct_ok
    # even with LWES 0.22.3, the "ERROR" message is non-fatal
    # the next version (as of lwes trunk r344) will no longer
    # spew a non-fatal "ERROR" message.
    tdb = LWES::TypeDB.new("#{File.dirname(__FILE__)}/meta_only.esf")
    classes = tdb.create_classes! :sparse => true
    assert_equal %w(Meta::Info::Only), classes.map { |x| x.to_s }
    Object.__send__ :remove_const, :Meta
  end

  def teardown
    new_classes = LWES::Event::CLASSES
    new_classes.each_key do |k|
      begin
        object.const_get k
        Object.__send__ :remove_const, k.to_sym
      rescue NameError
      end
    end
    new_classes.clear
  end
end
