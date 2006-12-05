#!/usr/bin/ruby

# load the FAM bindings
require 'fam'

# connect to fam
# (tests Fam::Connection::new)
fam = Fam::Connection.new

# set up unique temporary directory
temp = temp_base = ENV['TMPDIR'] || '/tmp'
temp = temp_base + '/famtest-' + rand(10000).to_s while test ?e, temp
system "mkdir #{temp}"

# create a thread which writes to the test file every second (so we can test
# the fam changed events)
writer_thread = Thread.new {
  # open the temporary output file -- we need to do this as a pipe, because
  # for some reason FAM doesn't see modifications made by _this_ process as
  # event-worthy.
  unless test_file = File.popen("cat > #{temp}/test.txt", 'w')
    $stderr.puts "Couldn't open \"#{temp}/test.txt\" for writing: " << $!
    exit 1
  end

  # loop forever writing to the file
  loop {
    test_file.puts 'test string: ' << rand.to_s
    sleep 1
  }
}

# monitor the file and a random file in the directory
# (tests Fam::Connection#monitor_dir and Fam::Connection#monitor_file)
dir_request = fam.monitor_dir temp
file_request = fam.monitor_file "#{temp}/test.txt"

# run event loop for at 10 seconds
# (tests Fam::Connection#pending?, Fam::Connection#next_event, and
# Fam::Event#to_s)
puts 'Starting first event loop.'
start_time = Time.now
while Time.now < start_time + 10
  next unless fam.pending?
  ev = fam.next_event
  puts ev.to_s
end

# stop monitoring the file
# (tests Fam::Connection#suspend)
# (this doesn't work under Gamin, see README)
begin
  fam.suspend file_request
rescue Fam::Error => d
  $stderr.puts "fam.suspend FAILED (are you using Gamin?): #{d}"
end

# cancel the directory monitor
# (tests Fam::Connection#cancel)
fam.cancel dir_request

# run the event loop for another 10 seconds
# (tests Fam::Connection#pending?, Fam::Connection#next_event, and
# Fam::Event#to_s)
puts 'Starting second event loop.'
start_time = Time.now
while Time.now < start_time + 10
  next unless fam.pending?
  ev = fam.next_event
  puts ev.to_s
end

# remove the temp directory
puts 'Cleaning up...'
system "rm -rf #{temp}"
