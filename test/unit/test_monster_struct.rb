require "#{File.expand_path(File.dirname(__FILE__))}/../test_helper"
require "set"

class TestMonsterStruct < Test::Unit::TestCase
  ESF_FILE = "#{File.dirname(__FILE__)}/test3.esf"

  def test_new_with_type_db
    type_db = LWES::TypeDB.new(ESF_FILE)
    classes = type_db.create_classes!(:parent => nil)
    members = classes[0].members
    assert members.size > 50
    assert_equal "Monster", classes[0]::NAME
    classes = type_db.create_classes!(:parent => nil, :sparse => true)
    assert_equal "Monster", classes[0]::NAME
    sparse_members = Set.new(classes[0].new.methods)
    members.each do |m|
      assert sparse_members.include?(m)
    end
  end
end
