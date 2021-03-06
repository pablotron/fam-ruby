require 'mkmf'

# i'd really prefer to do this with a fam-config, but as far as I know there
# isn't one (at least not one provided with Debian)

if have_library('fam', 'FAMOpen')
  have_func('rb_define_alloc_func', 'ruby.h')
  have_func('FAMDebugLevel', 'fam.h')
  have_func('FAMSuspendMonitor', 'fam.h')
  have_func('FAMResumeMonitor', 'fam.h')
  have_func('FAMMonitorCollection', 'fam.h')
  have_func('FAMNoExists', 'fam.h')
  $LDFLAGS << ' -lfam'
  create_makefile("fam")
end

