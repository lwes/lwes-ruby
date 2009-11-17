require 'rake/testtask'

desc "run all tests"
task :test => %w(test:unit)

desc "run unit tests"
Rake::TestTask.new('test:unit') do |t|
  t.libs << "ext/lwes"
  t.test_files = FileList['test/unit/test_*.rb']
  t.verbose = true if ENV["VERBOSE"].to_i > 0
end
