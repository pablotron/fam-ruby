/************************************************************************/
/* Copyright (c) 2002 Paul Duncan                                       */
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

#define VERSION "0.1.1"
#define UNUSED(x) ((void) (x))

static VALUE mFam,
             mDebug,
             cConn,
             cReq,
             cEvent;

static void dont_free(void *value) {
  UNUSED(value);
}

/*******************/
/* REQUEST METHODS */
/*******************/
static VALUE wrap_req(FAMRequest *req) {
  return Data_Wrap_Struct(cReq, 0, dont_free, req);
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
static VALUE fam_req_num(VALUE self) {
  FAMRequest *req;

  Data_Get_Struct(self, FAMRequest, req);
  return INT2NUM(FAMREQUEST_GETREQNUM(req));
}

/*****************/
/* EVENT METHODS */
/*****************/
static VALUE wrap_ev(FAMEvent *ev) {
  return Data_Wrap_Struct(cEvent, 0, free, ev);
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
static VALUE fam_ev_host(VALUE self) {
  FAMEvent *ev;

  Data_Get_Struct(self, FAMEvent, ev);

  if (ev->hostname && strlen(ev->hostname))
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
static VALUE fam_ev_file(VALUE self) {
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
static VALUE fam_ev_code(VALUE self) {
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
static VALUE fam_ev_req(VALUE self) {
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
static VALUE fam_ev_to_s(VALUE self) {
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
static void fam_conn_free(void *conn) {
  if (conn) {
    FAMClose((FAMConnection*) conn);
    free(conn);
  }
}

/*
 * Create a new connection to the FAM daemon.
 *
 * Aliases:
 *   Fam::Connection.open
 *   Fam::Connection.open2
 *
 * Examples:
 *   # connect and tell FAM the application is named 'foo'
 *   fam = Fam::Connection.new 'foo'
 *
 *   # just connect
 *   fam = Fam::Connection.new
 *
 */
VALUE fam_conn_new(int argc, VALUE *argv, VALUE klass) {
  FAMConnection *conn;
  VALUE self;
  int err = 0;

  conn = malloc(sizeof(FAMConnection));
  memset(conn, 0, sizeof(FAMConnection));

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
    char errstr[1024];

    snprintf(errstr, 1024, "Couldn't open FAM connection: %s",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }
  
  self = Data_Wrap_Struct(klass, 0, fam_conn_free, conn);
  rb_obj_call_init(self, 0, NULL);
  
  return self;
}

/*
 * Constructor for Fam::Connection object.
 *
 * This method is currently empty.  You should never call this method
 * directly unless you're instantiating a derived class (ie, you know
 * what you're doing).
 *
 */
static VALUE fam_conn_init(VALUE self) {
  return self;
}

/*
 * Close a Fam::Connection.
 *
 * Examples:
 *   fam.close
 *
 */
  
/* this causes a segfault, since ruby attempts to close the connection
 * when it goes out of scope.  We'll let ruby take care of it. :) */
/* static VALUE fam_conn_close(VALUE self) {
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMClose(conn);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024, "Couldn't close FAM connection: %s",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    
    rb_raise(rb_eException, errstr);
  }
  
  return self;
}*/

/*
 * Monitor a directory.
 *
 * Returns a Fam::Request object, which is used to identify the monitor
 * associated with events.
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
static VALUE fam_conn_dir(VALUE self, VALUE dir) {
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = malloc(sizeof(FAMRequest));
  err = FAMMonitorDirectory2(conn, RSTRING(dir)->ptr, req);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't monitor directory \"%s\": %s",
             RSTRING(dir)->ptr ? RSTRING(dir)->ptr : "NULL",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return wrap_req(req);
}

/*
 * Monitor a file.
 *
 * Returns a Fam::Request object, which is used to identify the monitor
 * associated with events.
 *
 * Aliases:
 *   Fam::Connection#file
 *
 * Examples:
 *   req = fam.monitor_file '/var/log/messages'
 *
 */
static VALUE fam_conn_file(VALUE self, VALUE file) {
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = malloc(sizeof(FAMRequest));
  err = FAMMonitorFile(conn, RSTRING(file)->ptr, req, NULL);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't monitor file \"%s\": %s",
             RSTRING(file)->ptr ? RSTRING(file)->ptr : "NULL", 
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return wrap_req(req);
}

/*
 * Monitor a collection.
 *
 * Aliases:
 *   Fam::Collection#monitor_col
 *   Fam::Collection#col
 *
 * Examples:
 *   req = fam.monitor_col 'download/images', 1, '*.jpg'
 *
 */
static VALUE fam_conn_col(VALUE self, VALUE col, VALUE depth, VALUE mask) {
  FAMConnection *conn;
  FAMRequest *req = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  req = malloc(sizeof(FAMRequest));
  err = FAMMonitorCollection(conn,
                             RSTRING(col)->ptr,
                             req,
                             NULL,
                             NUM2INT(depth),
                             RSTRING(mask)->ptr);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't monitor collection [\"%s\", %ld, \"%s\"]: %s",
             RSTRING(col)->ptr ? RSTRING(col)->ptr : "NULL",
             NUM2INT(depth),
             RSTRING(mask)->ptr ? RSTRING(mask)->ptr : "NULL",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return wrap_req(req);
}

/*
 * Suspend (stop monitoring) a monitor request.
 *
 * Aliases:
 *   Fam::Connection#suspend
 *
 * Examples:
 *   fam.suspend_monitor req
 *   fam.suspend req
 *
 */
static VALUE fam_conn_suspend(VALUE self, VALUE request) {
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMSuspendMonitor(conn, req);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't suspend monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req),
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return self;
}

/*
 * Resume (start monitoring) a monitor request.
 *
 * Aliases:
 *   Fam::Connection#resume
 *
 * Examples:
 *   fam.resume_monitor req
 *   fam.resume req
 *
 */
static VALUE fam_conn_resume(VALUE self, VALUE request) {
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMResumeMonitor(conn, req);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't resume monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req),
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return self;
}

/*
 * Cancel a monitor request.
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
static VALUE fam_conn_cancel(VALUE self, VALUE request) {
  FAMConnection *conn;
  FAMRequest *req;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  Data_Get_Struct(request, FAMRequest, req);
  err = FAMCancelMonitor(conn, req);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024,
             "Couldn't cancel monitor request %d: %s",
             FAMREQUEST_GETREQNUM(req),
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return self;
}

/*
 * Get the next event from the event queue, or block until an event is
 * available.
 *
 * Aliases:
 *   Fam::Connection#next_ev
 *   Fam::Connection#ev
 *
 * Examples:
 *   ev = fam.next_event
 *
 */
static VALUE fam_conn_next_ev(VALUE self) {
  FAMConnection *conn;
  FAMEvent *ev = NULL;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  ev = malloc(sizeof(FAMEvent));
  err = FAMNextEvent(conn, ev);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024, "Couldn't get next FAM event: %s",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return wrap_ev(ev);
}

/*
 * Are there any events in the queue?
 *
 * Aliases:
 *   Fam::Connection#pending
 *
 * Examples:
 *   puts 'no events pending' unless fam.pending?
 *
 */
static VALUE fam_conn_pending(VALUE self) {
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMPending(conn);

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024, "Couldn't check for pending FAM events: %s",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return (err > 0) ? Qtrue : Qfalse;
}

/*
 * Set the debug level of a Fam::Connection object.
 *
 * Note: This method is implemented in the bindings, but not in FAM
 * itself.
 *
 * Aliases:
 *   Fam::Connection#debug
 *
 * Examples:
 *   fam.debug = Fam::Debug::VERBOSE
 *
 */
static VALUE fam_conn_set_debug(VALUE self, VALUE level) {
  FAMConnection *conn;
  int err;

  Data_Get_Struct(self, FAMConnection, conn);
  err = FAMDebugLevel(conn, NUM2INT(level));

  if (err == -1) {
    char errstr[1024];

    snprintf(errstr, 1024, "Couldn't set debug level: %s",
             FamErrlist[FAMErrno] ? FamErrlist[FAMErrno] : "Unknown error");
    rb_raise(rb_eException, errstr);
  }

  return self;
}

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
static VALUE fam_conn_fd(VALUE self) {
  FAMConnection *conn;

  Data_Get_Struct(self, FAMConnection, conn);
  return INT2FIX(FAMCONNECTION_GETFD(conn));
}

void Init_fam(void) {
  mFam = rb_define_module("Fam");

  rb_define_const(mFam, "VERSION", rb_str_new2(VERSION));
  
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
  cConn = rb_define_class_under(mFam, "Connection", rb_cObject);
  
  rb_define_singleton_method(cConn, "new", fam_conn_new, -1);
  rb_define_singleton_method(cConn, "open", fam_conn_new, -1);
  rb_define_singleton_method(cConn, "open2", fam_conn_new, -1);

  rb_define_method(cConn, "initialize", fam_conn_init, 0);
  
  rb_define_method(cConn, "monitor_directory", fam_conn_dir, 1);
  rb_define_alias(cConn, "monitor_dir", "monitor_directory");
  rb_define_alias(cConn, "directory", "monitor_directory");
  rb_define_alias(cConn, "dir", "monitor_directory");

  rb_define_method(cConn, "monitor_file", fam_conn_file, 1);
  rb_define_alias(cConn, "file", "monitor_file");

  rb_define_method(cConn, "monitor_collection", fam_conn_col, 2);
  rb_define_alias(cConn, "monitor_col", "monitor_collection");
  rb_define_alias(cConn, "collection", "monitor_collection");
  rb_define_alias(cConn, "col", "monitor_collection");

  rb_define_method(cConn, "suspend_monitor", fam_conn_suspend, 1);
  rb_define_alias(cConn, "suspend", "suspend_monitor");

  rb_define_method(cConn, "resume_monitor", fam_conn_resume, 1);
  rb_define_alias(cConn, "resume", "resume_monitor");

  rb_define_method(cConn, "cancel_monitor", fam_conn_cancel, 1);
  rb_define_alias(cConn, "cancel", "cancel_monitor");

  rb_define_method(cConn, "next_event", fam_conn_next_ev, 0);
  rb_define_alias(cConn, "next_ev", "next_event");
  rb_define_alias(cConn, "ev", "next_event");

  rb_define_method(cConn, "pending?", fam_conn_pending, 0);
  rb_define_alias(cConn, "pending", "pending?");

  rb_define_method(cConn, "debug_level=", fam_conn_set_debug, 1);
  rb_define_alias(cConn, "debug=", "debug_level=");
  
  rb_define_method(cConn, "fd", fam_conn_fd, 0);
  rb_define_alias(cConn, "get_descriptor", "fd");
  rb_define_alias(cConn, "descriptor", "fd");
  rb_define_alias(cConn, "get_fd", "fd");

  /**********************/
  /* define Event class */
  /**********************/
  cEvent = rb_define_class_under(mFam, "Event", rb_cObject);

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
  cReq = rb_define_class_under(mFam, "Request", rb_cObject);

  rb_define_method(cReq, "reqnum", fam_req_num, 0);
  rb_define_alias(cReq, "request_number", "reqnum");
  rb_define_alias(cReq, "request_num", "reqnum");
  rb_define_alias(cReq, "request", "reqnum");
  rb_define_alias(cReq, "req_num", "reqnum");
  rb_define_alias(cReq, "req", "reqnum");
  rb_define_alias(cReq, "num", "reqnum");
}
