require 'mkmf'

# i'd really prefer to do this with a fam-config, but as far as I know there
# isn't one (at least not one provided with Debian)

if have_library('fam', 'FAMOpen')
  have_func('rb_define_alloc_func', 'ruby.h')
  $LDFLAGS << ' -lfam'
  create_makefile("fam")
end

