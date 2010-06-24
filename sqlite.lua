-- High-level interface to SQLite's C interface.

local require = require
module(..., package.seeall)

require "sqlitec"
local S = sqlitec

local db_mt = {
   exec = S.db_exec,
   get_table = S.db_get_table,
   errcode = S.db_errcode,
   errmsg = S.db_errmsg,
   prepare = S.db_prepare,
   close = S.db_close,
   __tostring = S.db__tostring,
   __gc = S.db__gc
}


local stmt_mt = {
   step = S.stmt_step,
   reset = S.stmt_reset,
   bind_double = S.stmt_bind_double,
   bind_int = S.stmt_bind_int,
   bind_null = S.stmt_bind_null,
   bind_text = S.stmt_bind_text,
   bind_param_count = S.stmt_bind_param_count,
   bind_param_idx = S.stmt_bind_param_index,
   col_double = S.stmt_col_double,
   col_int = S.stmt_col_int,
   col_text = S.stmt_col_text,
   col_type = S.stmt_col_type,
   __tostring = S.stmt__tostring,
   __gc = S.stmt__gc,
}


function open(fname)
   fname = fname or ":memory:"
   local db = S.open(fname)
end
