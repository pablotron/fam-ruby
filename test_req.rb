#!/usr/bin/env ruby

require 'fam'

# instantiate FAM connection
fam = Fam::Connection.new $0
rs = []

begin 
  # set up file monitors
  %w{A B C}.each { |c| puts path = '/tmp/' << (c * 5); rs << fam.file(path) }

  loop {
    while fam.pending?
      ev = fam.ev
      puts 'ev: '
      p ev
      p ev.req
      sleep 0.5
    end

    sleep 5
  }
ensure
  # cancel each request
  rs.each { |rq| fam.cancel rq }
end

