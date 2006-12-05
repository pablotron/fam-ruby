/************************************************************************/
/* Copyright (c) 2002-2006 Paul Duncan                                  */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation files */
/* (the "Software"), to deal in the Software without restriction,       */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies of the Software, its documentation and        */
/* marketing & publicity materials, and acknowledgment shall be given   */
/* in the documentation, materials and software packages that this      */
/* Software was used.                                                   */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY     */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE    */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.               */
/************************************************************************/

#include <ruby.h>
#include <fam.h>

/* fam.h in gamin doesn't have these */
#ifndef FAM_DEBUG_OFF
#define FAM_DEBUG_OFF 0
#define FAM_DEBUG_ON  1
#define FAM_DEBUG_VERBOSE 2
#endif

#define VERSION "0.2.0"
#define UNUSED(x) ((void) (x))

static VALUE mFam;
static VALUE mDebug;
static VALUE cConn;
static VALUE cReq;
static VALUE cEvent;
static VALUE eError;

static const char *
fam_error(void)
{
  const char *err = FamErrlist[FAMErrno];
  if (err) return err;
  return "Unknown error";
}

/*******************/
/* REQUEST METHODS */
/*******************/
static VALUE wrap_req(FAMRequest *req)
{
  return Data_Wrap_Struct(cReq, 0, 0, req);
}

/*
 * Return the request number of a Fam::Request object.
 *
 * Aliases:
 *   Fam::Request#request_number
 *   Fam::Request#request
 *   Fam::Request#req
 *
 * Examples:
 *   num = req.request
 *
 */
static VALUE fam_req_num(VALUE self)
{
  FAMRequest *req;

  Data_Get_Struct(self, FAMRequest, req);
  return INT2NUM(FAMREQUEST_GETREQNUM(req));
}

/*****************/
/* EVENT METHODS */
/*****************/
static VALUE wrap_ev(FAMEvent *ev)
{
  return Data_Wrap_Struct(cEvent, 0, -1, ev);
}

/*
 * Return the hostname of a Fam::Event object.
 *
 * Note: some versions of FAM (ex. the Debian package) ship without
 * remote support.  In those instances, this method will always return
 * "localhost".
 *
 * Aliases:
 *   Fam::Event#host
 *
 * Examples:
 *   host = ev.hostname
 *   host = ev.host
 *
 */
static VALUE fam_ev_host(VALUE self)
{
  FAMEvent *ev;

  Data_Get_Struct(self, FAMEvent, ev);

  if (ev->hostname && *ev->hostname)
    return rb_str_new2(ev->hostname);
  else 
    return rb_str_new2("localhost");
}

/*
 * Return the filename of a Fam::Event object.
 *
 * Note: for directory monitors, this method returns the path of the
 * file relative to the monitor directory, not the full path.
 * 
 * Aliases:
 *   Fam::Event#file
 *
 * Examples:
 *   path = ev.filename
 *   path = ev.file
 *
 */
static VALUE fam_ev_file(VALUE self)
{
  FAMEvent *ev;

  Data_Get_Struct(self, FAMEvent, ev);
  return rb_str_new2(ev->filename);
}

/*
 * Return the code (type) of a Fam::Event object.
 *
 * Note: see the file doc/event_codes.txt for a brief description of
 * each event code.
 *
 * Examples:
 *   code = ev.code
 *
 */
static VALUE fam_ev_code(VALUE self)
{
  FAMEvent *ev;

  Data_Get_Struct(self, FAMEvent, ev);
  return INT2FIX(ev->code);
}

/*
 * Return the request number of a Fam::Event object.
 *
 * Aliases:
 *   Fam::Event#request_number
 *   Fam::Event#request_num
 *   Fam::Event#request
 *   Fam::Event#req_num
 *   Fam::Event#req
 *   Fam::Event#num
 *
 * Examples:
 *   req = ev.request_number
 *
 */
static VALUE fam_ev_req(VALUE self)
{
  FAMEvent *ev;

  Data_Get_Struct(self, FAMEvent, ev);
  return INT2NUM(FAMREQUEST_GETREQNUM(&(ev->fr)));
}

/*
 * Return a human-readable string-representation of a Fam::Event object.
 *
 * Examples:
 *   puts 'event: ' << ev.to_s
 *
 */
static VALUE fam_ev_to_s(VALUE self)
{
  FAMEvent *ev;
  char str[1024];
  static char *ev_code_list[] = {
    "Unknown",
    "Changed",
    "Deleted",
    "StartExecuting",
    "StopExecuting",
    "Created",
    "Moved",
    "Acknowledge",
    "Exists",
    "EndExists",
  };

  Data_Get_Struct(self, FAMEvent, ev);
  snprintf(str, 1024, "%s \"%s\" (%d)",
           ev_code_list[ev->code],
           ev->filename,
           FAMREQUEST_GETREQNUM(&(ev->fr)));

  return rb_str_new2(str);
}

/**********************/
/* CONNECTION METHODS */
/**********************/
static void fam_conn_free(void *conn)
{
  FAMClose((FAMConnection*) conn);
  xfree(conn);
}

static VALUE fam_conn_s_alloc(VALUE klass)
{
  FAMConnection *conn = ALLOC(FAMConnection);
  memset(conn, 0, sizeof(FAMConnection));
  return Data_Wrap_Struct(klass, 0, fam_conn_free, conn);
}

#ifndef HAVE_RB_DEFINE_ALLOC_FUNC
/*
 * Create a new connection to the FAM daemon.
 *
 * Examples:
 *   # connect and tell FAM the application is named 'foo'
 *   fam = Fam::Connection.new 'foo'
 *
 *   # just connect
 *   fam = Fam::Connection.new
 *
 */
static VALUE fam_conn_s_new(int argc, VALUE *argv, VALUE klass)
{
  VALUE self = fam_conn_s_alloc(klass);

  rb_obj_call_init(self, argc, argv);
  return self;
}
#endif

static VALUE conn_close(VALUE self)
{
  return rb_funcall(self, rb_intern("close"), 0, 0);
}

/*
 * Create a new connection to the FAM daemon.
 *
 * Aliases:
 *   Fam::Connection.open2
 *
 * Examples:
 *   # connect and tell FAM the application is named 'foo'
 *   fam = Fam::Connection.open 'foo'
 *
 *   # just connect
 *   fam = Fam::Connection.open
 *
 *   # connect and close automatically
 *   Fam::Connection.open('foo') {|fam| ... }
 *
 */
static VALUE
fam_conn_s_open(int argc, VALUE *argv, VALUE klass)
{
  VALUE self = rb_class_new_instance(argc, argv, klass);

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, self, conn_close, self);
  }

  return self;
}

/*
 * Create a new connection to the FAM daemon.
 *
 * Raises an ArgumentError exception if the number of arguments is not 0
 * or 1, or a Fam::Error exception if a connection to FAM could not
 * be established.
 *
 * Examples:
 *   # connect and tell FAM the application is named 'foo'
 *   fam = Fam::Connection.new 'foo'
 *
 *   # just connect
 *   fam = Fam::Connection.new
 *
 */
static VALUE fam_conn_init(int argc, VALUE *argv, VALUE self)
{
  FAMConnection *conn;
  int err = 0;

  Data_Get_Struct(self, FAMConnection, conn);
  switch (argc) {
    case 0:
      err = FAMOpen(conn);
      break;
    case 1:
      err = FAMOpen2(conn, RSTRING(argv[0])->ptr);
      break;
    default:
      rb_raise(rb_eArgError, "invalid argument count (not 0 or 1)");
  }
  
  if (err == -1) {
    rb_raise(eError, "Couldn't open FAM connection: %s", fam_error());
  }

  return self;
}

/*
 * Close a Fam::Connection.
 *
 * Raises a Fam::Error exception if the connection to FAM could not
 * be closed.
 *
 * Examples:
 *   fam.close
 *
 */
  
/* this causes a segfault, since ruby attempts to close the connection
 * when it goes out of scope.  We'll let ruby take care of it. :) */
static VALUE fam_conn_close(VALUE self)
{
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMClose(conn);
  DATA_PTR(self) = NULL;

  if (err == -1) {
    rb_raise(eError, "Couldn't close FAM connection: %s", fam_error());
  }
  
  return self;
}

/*
 * Monitor a directory.
 *
 * Returns a Fam::Request object, which is used to identify the monitor
 * associated with events.
 *
 * Raises a Fam::Error exception if the directory could not be
 * monitored.
 *
 * Aliases:
 *   Fam::Connection#monitor_dir
 *   Fam::Connection#directory
 *   Fam::Connection#dir
 *
 * Examples:
 *   req = fam.monitor_directory '/tmp'
 *
 */
static VALUE fam_conn_dir(VALUE self, VALUE dir)
{
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = ALLOC(FAMRequest);
  err = FAMMonitorDirectory(conn, RSTRING(dir)->ptr, req, NULL);

  if (err == -1) {
    xfree(req);
    rb_raise(eError, "Couldn't monitor directory \"%s\": %s",
             RSTRING(dir)->ptr ? RSTRING(dir)->ptr : "NULL", fam_error());
  }

  return wrap_req(req);
}

/*
 * Monitor a file.
 *
 * Returns a Fam::Request object, which is used to identify the monitor
 * associated with events.
 *
 * Raises a Fam::Error exception if the file could not be monitored.
 *
 * Aliases:
 *   Fam::Connection#file
 *
 * Examples:
 *   req = fam.monitor_file '/var/log/messages'
 *
 */
static VALUE fam_conn_file(VALUE self, VALUE file)
{
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = ALLOC(FAMRequest);
  FAMREQUEST_GETREQNUM(req) = (int) req;
  err = FAMMonitorFile(conn, RSTRING(file)->ptr, req, NULL);

  if (err == -1) {
    xfree(req);
    rb_raise(eError, "Couldn't monitor file \"%s\": %s",
             RSTRING(file)->ptr ? RSTRING(file)->ptr : "NULL", fam_error());
  }

  return wrap_req(req);
}

#ifdef HAVE_FAMMONITORCOLLECTION
/*
 * Monitor a collection.
 *
 * Raises a Fam::Error exception if the collection could not be
 * monitored.  Note that this method exists under Gamin, but does not
 * actually do anything.
 *
 * Aliases:
 *   Fam::Collection#monitor_col
 *   Fam::Collection#col
 *
 * Examples:
 *   req = fam.monitor_col 'download/images', 1, '*.jpg'
 *
 */
static VALUE fam_conn_col(VALUE self, VALUE col, VALUE depth, VALUE mask)
{
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = ALLOC(FAMRequest);
  FAMREQUEST_GETREQNUM(req) = (int) req;
  err = FAMMonitorCollection(conn,
                             RSTRING(col)->ptr,
                             req,
                             NULL,
                             NUM2INT(depth),
                             RSTRING(mask)->ptr);

  if (err == -1) {
    xfree(req);
    rb_raise(eError, "Couldn't monitor collection [\"%s\", %d, \"%s\"]: %s",
             RSTRING(col)->ptr ? RSTRING(col)->ptr : "NULL",
             NUM2INT(depth),
             RSTRING(mask)->ptr ? RSTRING(mask)->ptr : "NULL",
	     fam_error());
  }

  return wrap_req(req);
}
#endif /* HAVE_FAMMONITORCOLLECTION */

#ifdef HAVE_FAMSUSPENDMONITOR
/*
 * Suspend (stop monitoring) a monitor request.
 *
 * Raises a Fam::Error exception if the monitor request could not be
 * suspended.  Note that this method exists under Gamin, but does not
 * actually do anything.
 * 
 * Aliases:
 *   Fam::Connection#suspend
 *
 * Examples:
 *   fam.suspend_monitor req
 *   fam.suspend req
 *
 */
static VALUE fam_conn_suspend(VALUE self, VALUE request)
{
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMSuspendMonitor(conn, req);

  if (err == -1) {
    rb_raise(eError, "Couldn't suspend monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req), fam_error());
  }

  return self;
}
#endif /* HAVE_FAMSUSPENDMONITOR */

#ifdef HAVE_FAMRESUMEMONITOR
/*
 * Resume (start monitoring) a monitor request.
 *
 * Raises a Fam::Error exception if the monitor request could not be
 * resumed.  Note that this method exists under Gamin, but does not
 * actually do anything.
 * 
 * Aliases:
 *   Fam::Connection#resume
 *
 * Examples:
 *   fam.resume_monitor req
 *   fam.resume req
 *
 */
static VALUE fam_conn_resume(VALUE self, VALUE request)
{
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMResumeMonitor(conn, req);

  if (err == -1) {
    rb_raise(eError, "Couldn't resume monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req), fam_error());
  }

  return self;
}
#endif /* HAVE_FAMRESUMEMONITOR */

/*
 * Cancel a monitor request.
 *
 * Raises a Fam::Error exception if the monitor request could not be
 * cancelled.
 *
 * Note: this method invalidates the specified monitor request.
 *
 * Aliases:
 *   Fam::Connection#cancel
 *
 * Examples:
 *   fam.cancel_monitor req
 *   fam.cancel req
 *
 */
static VALUE fam_conn_cancel(VALUE self, VALUE request)
{
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMCancelMonitor(conn, req);

  if (err == -1) {
    rb_raise(eError, "Couldn't cancel monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req), fam_error());
  }

  return self;
}

/*
 * Get the next event from the event queue, or block until an event is
 * available.
 *
 * Raises a Fam::Error exception if FAM couldn't check for pending
 * events, or if FAM-Ruby couldn't get the next FAM event.
 * 
 * Aliases:
 *   Fam::Connection#next_ev
 *   Fam::Connection#ev
 *
 * Examples:
 *   ev = fam.next_event
 *
 */
static VALUE fam_conn_next_ev(VALUE self)
{
  FAMConnection *conn;
  FAMEvent *ev = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);

  if (!(err = FAMPending(conn))) {
    int fd = FAMCONNECTION_GETFD(conn);
    fd_set rfds;

    FD_ZERO(&rfds);
    do {
      if (err == -1)
        rb_raise(eError, "Couldn't check for pending FAM events: %s", fam_error());
      FD_SET(fd, &rfds);
      rb_thread_select(fd + 1, &rfds, NULL, NULL, NULL);
    } while (!FD_ISSET(fd, &rfds) || !(err = FAMPending(conn)));
  }

  ev = ALLOC(FAMEvent);
  err = FAMNextEvent(conn, ev);

  if (err == -1) {
    xfree(ev);
    rb_raise(eError, "Couldn't get next FAM event: %s", fam_error());
  }

  return wrap_ev(ev);
}

/*
 * Are there any events in the queue?
 *
 * Raises a Fam::Error exception if FAM couldn't check for pending
 * events.
 *
 * Aliases:
 *   Fam::Connection#pending
 *
 * Examples:
 *   puts 'no events pending' unless fam.pending?
 *
 */
static VALUE fam_conn_pending(VALUE self)
{
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMPending(conn);

  if (err == -1) {
    rb_raise(eError, "Couldn't check for pending FAM events: %s", fam_error());
  }

  return (err > 0) ? Qtrue : Qfalse;
}

#ifdef HAVE_FAMDEBUGLEVEL
/*
 * Set the debug level of a Fam::Connection object.
 *
 * Raises a Fam::Error exception on failure.  Note that this method does
 * not exist under Gamin.
 *
 * Aliases:
 *   Fam::Connection#debug
 *
 * Examples:
 *   fam.debug = Fam::Debug::VERBOSE
 *
 */
static VALUE fam_conn_set_debug(VALUE self, VALUE level)
{
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);

  err = FAMDebugLevel(conn, NUM2INT(level));

  if (err == -1) {
    rb_raise(eError, "Couldn't set debug level: %s", fam_error());
  }

  return self;
}
#endif

/*
 * Get the file descriptor of a Fam::Connection object.
 *
 * Note: This method allows you to wait for FAM events using select()
 * instead of polling via Fam::Connection#pending and
 * Fam::Connection#next_event; see the second example below for more
 * information.
 *
 * Aliases:
 *   Fam::Connection#get_descriptor
 *   Fam::Connection#descriptor
 *   Fam::Connection#get_fd
 *
 * Examples:
 *   # simple use
 *   fd = fam.fd
 *
 *   # wrap the FAM connection descriptor in an IO object for use in a
 *   # select() call
 *   io = IO.new fam.fd, 'r'
 *   select [io], , , 10
 *   
 */
static VALUE fam_conn_fd(VALUE self)
{
  FAMConnection *conn;

  Data_Get_Struct(self, FAMConnection, conn);
  return INT2FIX(FAMCONNECTION_GETFD(conn));
}

#ifdef HAVE_FAMNOEXISTS
/*
 * Gamin-specific extension for FAM to not propagate Exists events on
 * directory monitoring startup. This speeds up watching large
 * directories but can introduce a mismatch between the FAM view of the
 * directory and the program own view.
 *
 * Has no effect if FAMNoExists is not available.
 *
 * Raises a Fam::Error exception if an error is encountered.
 * 
 * Examples:
 *   fam.no_exists
 *
 */
static VALUE fam_conn_no_exists(VALUE self)
{
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMNoExists(conn);

  if (err == -1) {
    rb_raise(eError, "Couldn't turn off exists events: %s",
             fam_error());
  }
  return self;
}
#endif

void Init_fam(void)
{
  mFam = rb_define_module("Fam");

  rb_define_const(mFam, "VERSION", rb_str_new2(VERSION));
  eError = rb_define_class_under(mFam, "Error", rb_eStandardError);

  /********************************/
  /* define Debug module          */
  /* (for connection debug level) */
  /********************************/
  mDebug = rb_define_module_under(mFam, "Debug");
  rb_define_const(mDebug, "OFF", INT2FIX(FAM_DEBUG_OFF));
  rb_define_const(mDebug, "ON", INT2FIX(FAM_DEBUG_ON));
  rb_define_const(mDebug, "VERBOSE", INT2FIX(FAM_DEBUG_VERBOSE));

  /***************************/
  /* define Connection class */
  /***************************/
  cConn = rb_define_class_under(mFam, "Connection", rb_cData);
  
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
  rb_define_alloc_func(cConn, fam_conn_s_alloc);
#else
  rb_define_singleton_method(cConn, "new", fam_conn_s_new, -1);
#endif
  rb_define_singleton_method(cConn, "open", fam_conn_s_open, -1);
  rb_define_singleton_method(cConn, "open2", fam_conn_s_open, -1);

  rb_define_method(cConn, "initialize", fam_conn_init, -1);
  rb_define_method(cConn, "close", fam_conn_close, 0);
  
  rb_define_method(cConn, "monitor_directory", fam_conn_dir, 1);
  rb_define_alias(cConn, "monitor_dir", "monitor_directory");
  rb_define_alias(cConn, "directory", "monitor_directory");
  rb_define_alias(cConn, "dir", "monitor_directory");

  rb_define_method(cConn, "monitor_file", fam_conn_file, 1);
  rb_define_alias(cConn, "file", "monitor_file");

#ifdef HAVE_FAMMONITORCOLLECTION
  rb_define_method(cConn, "monitor_collection", fam_conn_col, 2);
  rb_define_alias(cConn, "monitor_col", "monitor_collection");
  rb_define_alias(cConn, "collection", "monitor_collection");
  rb_define_alias(cConn, "col", "monitor_collection");
#endif /* HAVE_FAMMONITORCOLLECTION */

#ifdef HAVE_FAMSUSPENDMONITOR
  rb_define_method(cConn, "suspend_monitor", fam_conn_suspend, 1);
  rb_define_alias(cConn, "suspend", "suspend_monitor");
#endif /* HAVE_FAMSUSPENDMONITOR */

#ifdef HAVE_FAMRESUMEMONITOR
  rb_define_method(cConn, "resume_monitor", fam_conn_resume, 1);
  rb_define_alias(cConn, "resume", "resume_monitor");
#endif /* HAVE_FAMRESUMEMONITOR */

  rb_define_method(cConn, "cancel_monitor", fam_conn_cancel, 1);
  rb_define_alias(cConn, "cancel", "cancel_monitor");

  rb_define_method(cConn, "next_event", fam_conn_next_ev, 0);
  rb_define_alias(cConn, "next_ev", "next_event");
  rb_define_alias(cConn, "ev", "next_event");

  rb_define_method(cConn, "pending?", fam_conn_pending, 0);
  rb_define_alias(cConn, "pending", "pending?");

#ifdef HAVE_FAMDEBUGLEVEL
  rb_define_method(cConn, "debug_level=", fam_conn_set_debug, 1);
  rb_define_alias(cConn, "debug=", "debug_level=");
#endif /* HAVE_FAMDEBUGLEVEL */
  
  rb_define_method(cConn, "fd", fam_conn_fd, 0);
  rb_define_alias(cConn, "get_descriptor", "fd");
  rb_define_alias(cConn, "descriptor", "fd");
  rb_define_alias(cConn, "get_fd", "fd");

#ifdef HAVE_FAMNOEXISTS
  rb_define_method(cConn, "no_exists", fam_conn_no_exists, 0);
#endif /* HAVE_FAMNOEXISTS */

  /**********************/
  /* define Event class */
  /**********************/
  cEvent = rb_define_class_under(mFam, "Event", rb_cData);

  rb_define_method(cEvent, "hostname", fam_ev_host, 0);
  rb_define_alias(cEvent, "host", "hostname");

  rb_define_method(cEvent, "filename", fam_ev_file, 0);
  rb_define_alias(cEvent, "file", "filename");

  rb_define_method(cEvent, "code", fam_ev_code, 0);

  rb_define_method(cEvent, "reqnum", fam_ev_req, 0);
  rb_define_alias(cEvent, "request_number", "reqnum");
  rb_define_alias(cEvent, "request_num", "reqnum");
  rb_define_alias(cEvent, "request", "reqnum");
  rb_define_alias(cEvent, "req_num", "reqnum");
  rb_define_alias(cEvent, "req", "reqnum");
  rb_define_alias(cEvent, "num", "reqnum");
  
  rb_define_method(cEvent, "to_s", fam_ev_to_s, 0);

  /* define event codes */
  rb_define_const(cEvent, "CHANGED", INT2FIX(FAMChanged));
  rb_define_const(cEvent, "DELETED", INT2FIX(FAMDeleted));
  rb_define_const(cEvent, "START_EXECUTING", INT2FIX(FAMStartExecuting));
  rb_define_const(cEvent, "STOP_EXECUTING", INT2FIX(FAMStopExecuting));
  rb_define_const(cEvent, "CREATED", INT2FIX(FAMCreated));
  rb_define_const(cEvent, "MOVED", INT2FIX(FAMMoved));
  rb_define_const(cEvent, "ACKNOWLEDGE", INT2FIX(FAMAcknowledge));
  rb_define_const(cEvent, "ACK", INT2FIX(FAMAcknowledge));
  rb_define_const(cEvent, "EXISTS", INT2FIX(FAMExists));
  rb_define_const(cEvent, "END_EXIST", INT2FIX(FAMEndExist));
  
  /************************/
  /* define Request class */
  /************************/
  cReq = rb_define_class_under(mFam, "Request", rb_cData);

  rb_define_method(cReq, "reqnum", fam_req_num, 0);
  rb_define_alias(cReq, "request_number", "reqnum");
  rb_define_alias(cReq, "request_num", "reqnum");
  rb_define_alias(cReq, "request", "reqnum");
  rb_define_alias(cReq, "req_num", "reqnum");
  rb_define_alias(cReq, "req", "reqnum");
  rb_define_alias(cReq, "num", "reqnum");
}
