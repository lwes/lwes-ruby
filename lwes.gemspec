Gem::Specification.new do |s|
  s.name = %q{lwes}
  s.version = "0.1.0"
  s.date = Time.now
  s.authors = ["Erik S. Chang", "Frank Maritato"]
  s.email = %q{lwes-devel@lists.sourceforge.net}
  s.summary = %q{Ruby API for the Light Weight Event System}
  s.homepage = %q{http://www.lwes.org/}
  s.description = %q{
The LWES Light-Weight Event System is a framework for allowing the exchange of
information from many machines to many machines in a controlled, platform
neutral, language neutral way.  The exchange of information is done in a
connectless fashion using multicast or unicast UDP, and using self describing
data so that any platform or language can translate it to it's local dialect.
}.strip
  s.files = %w(
COPYING
README
Rakefile
ext/lwes/emitter.c
ext/lwes/extconf.rb
ext/lwes/lwes.c
ext/lwes/lwes_ruby.h
ext/lwes/memrchr.c
ext/lwes/numeric.c
ext/lwes/type_db.c
lib/lwes.rb
lib/lwes/emitter.rb
lib/lwes/struct.rb
test/test_helper.rb
test/unit/test1.esf
test/unit/test_emit_struct.rb
test/unit/test_emitter.rb
test/unit/test_struct.rb
test/unit/test_type_db.rb
)
end
