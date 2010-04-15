require 'rake/testtask'

desc "run all tests"
task :test => %w(test:unit)

desc "run unit tests"
Rake::TestTask.new('test:unit') do |t|
  t.libs << "ext/lwes"
  t.test_files = FileList['test/unit/test_*.rb']
  t.warning = true
  t.verbose = true if ENV["VERBOSE"].to_i > 0
end

gem 'rdoc', '>= 2.4.3'
require 'rdoc/task'
RDoc::Task.new do |rd|
  rd.main = "README"
  rd.rdoc_files.include("README", "lib/**/*.rb", "ext/lwes/*.c")
end
