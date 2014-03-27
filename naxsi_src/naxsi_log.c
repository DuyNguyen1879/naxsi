/*
 * NAXSI, a web application firewall for NGINX
 * Copyright (C) 2011, Thibault 'bui' Koechlin
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
** This files contains skeleton functions, 
** such as registred handlers. Readers already 
** aware of nginx's modules can skip most of this.
*/

#include "naxsi.h"


/**
 * configuration function for the "NaxsiLogfile" keyword in the 
 * ngx_command_t modules (check naxsi_skeleton.c)
 * for the MAIN
 */
char *
ngx_http_naxsi_logfile_main_conf(ngx_conf_t *cf, ngx_command_t *cmd,
                              void *conf)
{
  ngx_http_dummy_main_conf_t    *alcf = conf;
  ngx_str_t                     *value;
  ngx_naxsi_log_t               *log;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR);  /* alloc a new rule */

  value = cf->args->elts;
  /* create specific log file */
  if ( (!ngx_strcmp(value[0].data, TOP_NAXSI_LOGFILE_N) ||
        !ngx_strcmp(value[0].data, TOP_NAXSI_LOGFILE_T))
       && value[1].len) {
    
    if (alcf->naxsi_logs == NULL) {
      alcf->naxsi_logs = ngx_array_create(cf->pool, 2, sizeof(ngx_naxsi_log_t));
      if (alcf->naxsi_logs == NULL) {
        return NGX_CONF_ERROR;
      }
    }

  log = ngx_array_push(alcf->naxsi_logs);
  if (log == NULL) {
    return NGX_CONF_ERROR;
  }
  ngx_memzero(log, sizeof(ngx_naxsi_log_t));

  log->file = ngx_conf_open_file(cf->cycle, &value[1]);
  if (log->file == NULL) {
    return NGX_CONF_ERROR;
  }

  // log format

#ifdef mechanics_debug
 // ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "naxsi log.");
#endif
    return (NGX_CONF_OK);
  }
  else
    return NGX_CONF_ERROR;
}



/**
 * configuration function for the "NaxsiLogfile" keyword in the 
 * ngx_command_t modules (check naxsi_skeleton.c)
 * for LOCATION
 */
char *
ngx_http_naxsi_logfile_loc_conf(ngx_conf_t *cf, ngx_command_t *cmd,
                           void *conf)
{
  ngx_http_dummy_loc_conf_t     *alcf = conf;
  ngx_str_t                     *value;
  ngx_naxsi_log_t               *log;

  if (!alcf || !cf)
    return (NGX_CONF_ERROR);
  value = cf->args->elts;

  /* create specific log file */
  if ( (!ngx_strcmp(value[0].data, TOP_NAXSI_LOGFILE_N) ||
        !ngx_strcmp(value[0].data, TOP_NAXSI_LOGFILE_T))
       && value[1].len) {
    
    if (alcf->naxsi_logs == NULL) {
      alcf->naxsi_logs = ngx_array_create(cf->pool, 2, sizeof(ngx_naxsi_log_t));
      if (alcf->naxsi_logs == NULL) {
        return NGX_CONF_ERROR;
      }
    }

  log = ngx_array_push(alcf->naxsi_logs);
  if (log == NULL) {
    return NGX_CONF_ERROR;
  }
  ngx_memzero(log, sizeof(ngx_naxsi_log_t));

  log->file = ngx_conf_open_file(cf->cycle, &value[1]);
  if (log->file == NULL) {
    return NGX_CONF_ERROR;
  }

  // log format

#ifdef mechanics_debug
 // ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "naxsi log.");
#endif
    return (NGX_CONF_OK);
  }
  else
    return NGX_CONF_ERROR;
}



/**
 * low level function to write log. Code has been taken from 
 * nginx core/ngx_log.c 
 */
static void
ngx_naxsi_log_write(ngx_http_request_t *r, ngx_naxsi_log_t *log, u_char *buf, size_t len)
{
    u_char              *name;
    time_t               now;
    ssize_t              n;
    ngx_err_t            err;

    name = log->file->name.data;
    n = ngx_write_fd(log->file->fd, buf, len);
    if (n == (ssize_t) len) {
        return;
    }

    now = ngx_time();

    if (n == -1) {
        err = ngx_errno;

        if (err == NGX_ENOSPC) {
            log->disk_full_time = now;
        }

        if (now - log->error_log_time > 59) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, err,
                          ngx_write_fd_n " to \"%s\" failed", name);

            log->error_log_time = now;
        }

        return;
    }

    if (now - log->error_log_time > 59) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      ngx_write_fd_n " to \"%s\" was incomplete: %z of %uz",
                      name, n, len);

        log->error_log_time = now;
    }
}



/**
 * When dealing with a http request, nginx process it via different phases
 * (see http://wiki.nginx.org/Phases). Logging is one of the last phase.
 * the naxsi_http_log_handler is called to treat this phase.
 *
 * What naxsi_http_log_handler does: it check if the ngx_http_dummy_loc_conf_t has
 * one or several log strings to output (which have been filed via ngx_log_naxsi()),
 * and write them to the naxsi log file
 */
ngx_int_t
naxsi_http_log_handler(ngx_http_request_t *r)
{
  //ngx_http_request_ctx_t     *ctx;
  ngx_http_dummy_loc_conf_t  *cf;
  ngx_naxsi_log_t            *log;
  ngx_uint_t                 i,l;
  ngx_str_t                  *str;
  ngx_array_t                *logarray;
  ngx_http_dummy_main_conf_t *main_cf;
  
  cf = ngx_http_get_module_loc_conf(r, ngx_http_naxsi_module);
  main_cf = ngx_http_get_module_main_conf(r, ngx_http_naxsi_module);
  
  if (cf->naxsi_logstrings==NULL)
    return NGX_OK;
  
  if (cf->naxsi_logs!=NULL && cf->naxsi_logs->nelts > 0) {
    logarray=cf->naxsi_logs;
  } else if (main_cf->naxsi_logs!=NULL && main_cf->naxsi_logs->nelts > 0) {
    logarray=main_cf->naxsi_logs;
  } else {
//    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "in naxsi_http_log_handler log array is NULL");
    return NGX_ERROR;
  }
  log = logarray->elts;
  str=cf->naxsi_logstrings->elts;
  for (i=0;i<cf->naxsi_logstrings->nelts;i++) {
    for (l = 0; l < logarray->nelts; l++) {
      if (str[i].data!=NULL)
        ngx_naxsi_log_write(r, &log[l], str[i].data, str[i].len);
    }
    ngx_pfree(r->pool, str[i].data);
    str[i].data=NULL;
    str[i].len=0;
  }
  ngx_array_destroy(cf->naxsi_logstrings);
  //cf->naxsi_logstrings->nelts=0;
  cf->naxsi_logstrings=NULL;

  return NGX_OK;
}



static ngx_str_t err_levels[] = {
    ngx_null_string,
    ngx_string("emerg"),
    ngx_string("alert"),
    ngx_string("crit"),
    ngx_string("error"),
    ngx_string("warn"),
    ngx_string("notice"),
    ngx_string("info"),
    ngx_string("debug")
};



/**
 * the rest of naxsi should call this function (which is compatible with 
 * classical nginx log error function).
 * This function will check if a naxsi_log has been defined:
 * - if not, it will redirect logs to nginx log error
 * - if it exist, it will append the log to the loc->naxsi_logstrings, to
 *   be output at the end of the current request (see naxsi_http_log_handler())
 */
#if (NGX_HAVE_VARIADIC_MACROS)
void
ngx_log_naxsi(ngx_uint_t level, ngx_http_request_t *r, ngx_err_t err,
    const char *fmt, ...)
#else
void
ngx_log_naxsi(ngx_uint_t level, ngx_http_request_t *r, ngx_err_t err,
    const char *fmt, va_list args)
#endif
{
#if (NGX_HAVE_VARIADIC_MACROS)
    va_list  args;
#endif
    ngx_http_dummy_loc_conf_t *loc;
    ngx_http_dummy_main_conf_t *main_cf;
    u_char  *p, *last;
    u_char   errstr[NGX_MAX_ERROR_STR];
    ngx_str_t *logmsg;


    last = errstr + NGX_MAX_ERROR_STR;

    loc = ngx_http_get_module_loc_conf(r, ngx_http_naxsi_module);
    main_cf = ngx_http_get_module_main_conf(r, ngx_http_naxsi_module);

    if (loc  == NULL || r == NULL || main_cf==NULL) {
        return;
    }
    
    if ((loc->naxsi_logs==NULL || loc->naxsi_logs->nelts == 0) && (main_cf->naxsi_logs==NULL || main_cf->naxsi_logs->nelts == 0 )) {

#if (NGX_HAVE_VARIADIC_MACROS)
      va_start(args, fmt);
      p = ngx_vslprintf(errstr, last, fmt, args);
      *p='\0';
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, (const char *)errstr); 
      va_end(args);
#else
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, fmt, args); 
#endif
      return;
    }


    ngx_memcpy(errstr, ngx_cached_err_log_time.data,
               ngx_cached_err_log_time.len);

    p = errstr + ngx_cached_err_log_time.len;

    p = ngx_slprintf(p, last, " [%V] ", &err_levels[level]);

    /* pid#tid */
    p = ngx_slprintf(p, last, "%P#" NGX_TID_T_FMT ": ",
                    ngx_log_pid, ngx_log_tid);

    if (r->connection) {
//      if (r->connection->log) {
//        p = ngx_slprintf(p, last, "*%uA ", r->connection->log->connection);
//      }
      p = ngx_slprintf(p, last, "*%uA ", r->connection->number);
    }
#if (NGX_HAVE_VARIADIC_MACROS)

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

#else

    p = ngx_vslprintf(p, last, fmt, args);

#endif

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (level != NGX_LOG_DEBUG && r->connection) {
      if (r->connection->log) {
        if (r->connection->log->handler) {
          p = r->connection->log->handler(r->connection->log, p, last - p);
        }
      }
    }

    if (p > last - NGX_LINEFEED_SIZE -1) {
        p = last - NGX_LINEFEED_SIZE -1;
    }

    ngx_linefeed(p);
    *(p++)='\0';

    /* add new line to log afer */
    if (loc->naxsi_logstrings==NULL)
      loc->naxsi_logstrings = ngx_array_create(r->pool, 1, sizeof(ngx_str_t));
    
    if (loc->naxsi_logstrings==NULL) {
      return;
    }
    
    logmsg=ngx_array_push(loc->naxsi_logstrings);
    if (logmsg!=NULL) {
      logmsg->len=strlen((const char *)errstr);
      logmsg->data=ngx_pcalloc(r->pool, logmsg->len+1);
      if (logmsg->data!=NULL) {
        p=ngx_copy(logmsg->data,errstr,logmsg->len+1);
      } else {
        logmsg->len=0;
      }
    }
}
