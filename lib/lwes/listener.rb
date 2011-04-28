#
# This class is only supported on Ruby 1.9
#
#   listener = LWES::Listener.new :address => "224.1.1.11", :port => 12345
#   listener.each do |event|
#     p event
#   end
#
class LWES::Listener

  # we disallow dup-ing objects since GC could double-free otherwise
  def dup
    self
  end

  alias clone dup

  # processes each LWES::Event object as it is received, yielding
  # the LWES::Event to a given block
  def each
    begin
      yield recv
    rescue Errno::EINTR
    end while true
    self
  end
end
