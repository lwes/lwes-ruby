Gem::Specification.new do |s|
  s.name = %q{lwes}
  s.version = "0.7.0"
  s.date = Time.now
  s.authors = ["Erik S. Chang", "Frank Maritato"]
  s.email = %q{lwes-devel@lists.sourceforge.net}
  s.summary = %q{Ruby API for the Light Weight Event System}
  s.homepage = %q{http://www.lwes.org/}
  s.extensions = %w(ext/lwes_ext/extconf.rb)
  s.description = %q{
The LWES Light-Weight Event System is a framework for allowing the exchange of
information from many machines to many machines in a controlled, platform
neutral, language neutral way.  The exchange of information is done in a
connectless fashion using multicast or unicast UDP, and using self describing
data so that any platform or language can translate it to it's local dialect.
}.strip
  s.files = %w(
COPYING
ChangeLog
README
Rakefile
examples/demo.rb
examples/my_events.esf
ext/lwes_ext/emitter.c
ext/lwes_ext/event.c
ext/lwes_ext/extconf.rb
ext/lwes_ext/lwes.c
ext/lwes_ext/lwes_ruby.h
ext/lwes_ext/numeric.c
ext/lwes_ext/type_db.c
lib/lwes.rb
lib/lwes/emitter.rb
lib/lwes/event.rb
lib/lwes/struct.rb
lib/lwes/type_db.rb
lwes.gemspec
test/test_helper.rb
test/unit/meta_only.esf
test/unit/namespaced.esf
test/unit/test1.esf
test/unit/test2.esf
test/unit/test_emit_struct.rb
test/unit/test_emitter.rb
test/unit/test_event.rb
test/unit/test_struct.rb
test/unit/test_type_db.rb
) + %w(
ext/lwes_ext/lwes-0.23.1.tar.gz
)
  s.rubyforge_project = 'lwes'
  s.test_files = s.files.grep(%r{\Atest/unit/test_})
  s.add_development_dependency(%q<rake-compiler>, [">= 0.7.6"])
end
