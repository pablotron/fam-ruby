require 'rubygems'

spec = Gem::Specification.new do |s|

  #### Basic information.

  s.name = 'FAM-Ruby'
  s.version = '0.2.0'
  s.summary = <<-EOF
    FAM (SGI's File Alteration Monitor) and Gamin bindings for Ruby.
  EOF
  s.description = <<-EOF
    FAM (SGI's File Alteration Monitor) and Gamin bindings for Ruby.
  EOF

  s.requirements << 'FAM, version 2.6.6.1 (or newer) or Gamin'
  s.requirements << 'Ruby, version 1.6.7 (or newer)'

  #### Which files are to be included in this gem?  Everything!  (Except CVS directories.)

  s.files = Dir.glob("**/*").delete_if { |item| item.include?("CVS") }

  #### C code extensions.

  s.require_path = 'lib' # is this correct?
  s.extensions << "extconf.rb"

  #### Load-time details: library and application (you will need one or both).
  s.autorequire = 'fam'
  s.has_rdoc = true
  s.rdoc_options = ['--webcvs',
  'http://cvs.pablotron.org/cgi-bin/viewcvs.cgi/fam-ruby/', '--title',
  'FAM-Ruby API Documentation', 'fam.c', 'README', 'ChangeLog',
  'AUTHORS', 'COPYING', 'examples/dirmon.rb', 'examples/famtest.rb',
  'event_codes.txt']

  #### Author and project details.

  s.author = 'Paul Duncan'
  s.email = 'pabs@pablotron.org'
  s.homepage = 'http://pablotron.org/software/fam-ruby/'
  s.rubyforge_project = 'fam-ruby'

  # sign gems (strip from release)
  s.signing_key = '/home/pabs/.rubygems/rubygems.key'
  s.cert_chain = %w{ca rubygems}.map { |v| "/home/pabs/.rubygems/#{v}.crt" }
end
