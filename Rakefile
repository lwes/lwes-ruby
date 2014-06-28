require 'rake/testtask'
task :default => :test

desc "run all tests"
task :test => %w(test:unit)

desc "run unit tests"
Rake::TestTask.new('test:unit' => :compile) do |t|
  t.libs << "ext/lwes_ext"
  t.test_files = FileList['test/unit/test_*.rb']
  t.warning = true
  t.verbose = true if ENV["VERBOSE"].to_i > 0
end
require "rake/extensiontask"
Rake::ExtensionTask.new("lwes_ext")

require 'rdoc/task'
RDoc::Task.new do |rd|
  rd.main = "README"
  rd.rdoc_files.include("README", "lib/**/*.rb", "ext/lwes_ext/*.c")
end
