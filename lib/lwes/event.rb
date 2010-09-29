# this class is incomplete, but will eventually be subclassable
# like Struct in Ruby
class LWES::Event
  def inspect
    "#<#{self.class}:#{to_hash}>"
  end
end
