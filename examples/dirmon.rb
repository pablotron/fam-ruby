#!/usr/bin/ruby

#########################################################################
# dirmon.rb - monitor a directory for changes                           #
#                                                                       #
#########################################################################

# load the fam bindings
require 'fam'

# get a list of files in the specified path
path = ARGV[0] || Dir::pwd

# check to make sure specified directory exists
unless test ?d, path
  $stderr.puts 'ERROR: Specified directory does not exist.'
  $stderr.puts "usage: #$0 <directory>"
  exit 1
end

# open a connection to FAM and start monitoring specified path
fam = Fam::Connection.new $0
fam.monitor_dir path

# get a list of files in the directory
files = Dir["#{path}/*"].collect! { |f| File::basename f }

# method to print out command prompt
def prompt
  print '> '
  $stdout.flush
end

# print out info and prompt
puts 'Commands: ls, q, quit'
prompt

# only run for 2 minutes
start_time = Time.now
while Time.now < start_time + 120
  # if there are fam events pending, then process them
  if fam.pending?
    ev = fam.next_event
    case ev.code
      when Fam::Event::EXISTS
        files << ev.file unless ev.file == path
      when Fam::Event::CHANGED
        puts 'File changed: ' << ev.file
        prompt
      when Fam::Event::DELETED
        puts 'File deleted: ' << ev.file
        prompt
        files -= [ev.file]
      when Fam::Event::CREATED
        puts 'File created: ' << ev.file
        prompt
        files << ev.file
    end

  end

  # process standard input if there is a command pending
  if select([$stdin], nil, nil, 0.1)
    case cmd = $stdin.gets
      when /^\s*ls/i
        puts 'File list: ' << files.join(', ')
      when /^\s*quit/i
        exit 0
      when /^\s*q/i
        exit 0
      else
        puts 'Unknown command: ' << cmd
    end

    # print out a new command prompt
    prompt
  end
end

puts 'Done.'
