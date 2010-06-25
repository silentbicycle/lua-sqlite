#include <stdlib.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>
#include <sqlite3.h>
#include "lsqlite.h"
#include <assert.h>

#define SI static int


SI ver(lua_State *L) { lua_pushstring(L, sqlite3_libversion()); return 1; }
SI ver_num(lua_State *L) { lua_pushinteger(L, sqlite3_libversion_number()); return 1; }


SI pushres(lua_State *L, int res) {
        const char *s;
        switch (res) {
        case SQLITE_OK: s = "ok"; break;       /* Successful result */
        case SQLITE_ERROR: s = "error"; break; /* SQL error or missing database */
        case SQLITE_INTERNAL: s = "internal"; break; /* Internal logic error in SQLite */
        case SQLITE_PERM: s = "perm"; break;   /* Access permission denied */
        case SQLITE_ABORT: s = "abort"; break; /* Callback routine requested an abort */
        case SQLITE_BUSY: s = "busy"; break;   /* The database file is locked */
        case SQLITE_LOCKED: s = "locked"; break; /* A table in the database is locked */
        case SQLITE_NOMEM: s = "nomem"; break; /* A malloc() failed */
        case SQLITE_READONLY: s = "readonly"; break; /* Attempt to write a readonly database */
        case SQLITE_INTERRUPT: s = "interrupt"; break; /* Operation terminated by sqlite3_interrupt()*/
        case SQLITE_IOERR: s = "ioerr"; break; /* Some kind of disk I/O error occurred */
        case SQLITE_CORRUPT: s = "corrupt"; break; /* The database disk image is malformed */
        case SQLITE_NOTFOUND: s = "notfound"; break; /* NOT USED. Table or record not found */
        case SQLITE_FULL: s = "full"; break;   /* Insertion failed because database is full */
        case SQLITE_CANTOPEN: s = "cantopen"; break; /* Unable to open the database file */
        case SQLITE_PROTOCOL: s = "protocol"; break; /* NOT USED. Database lock protocol error */
        case SQLITE_EMPTY: s = "empty"; break; /* Database is empty */
        case SQLITE_SCHEMA: s = "schema"; break; /* The database schema changed */
        case SQLITE_TOOBIG: s = "toobig"; break; /* String or BLOB exceeds size limit */
        case SQLITE_CONSTRAINT: s = "constraint"; break; /* Abort due to constraint violation */
        case SQLITE_MISMATCH: s = "mismatch"; break; /* Data type mismatch */
        case SQLITE_MISUSE: s = "misuse"; break; /* Library used incorrectly */
        case SQLITE_NOLFS: s = "nolfs"; break; /* Uses OS features not supported on host */
        case SQLITE_AUTH: s = "auth"; break;   /* Authorization denied */
        case SQLITE_FORMAT: s = "format"; break; /* Auxiliary database format error */
        case SQLITE_RANGE: s = "range"; break; /* 2nd parameter to sqlite3_bind out of range */
        case SQLITE_NOTADB: s = "notadb"; break; /* File opened that is not a database file */
        case SQLITE_ROW: s = "row"; break;     /* sqlite3_step() has another row ready */
        case SQLITE_DONE: s = "done"; break;   /* sqlite3_step() has finished executing */
        default: s = "unknown"; break;
        }
        lua_pushstring(L, s);
        return 1;
}


/************/
/* Database */
/************/

SI open(lua_State *L) {
        size_t len;
        const char *fn = luaL_optlstring(L, 1, ":memory:", &len);
        sqlite3 **db;
        LuaSQLite *ldb;
        
        int res = sqlite3_open(fn, db);
        if (res == SQLITE_OK) {
                ldb = (LuaSQLite *)lua_newuserdata(L, sizeof(LuaSQLite));
                ldb->db = *db;
                luaL_getmetatable(L, "LuaSQLite");
                lua_setmetatable(L, -2);
                return 1;
        } else {
                LERROR(sqlite3_errmsg(*db));
                return 0;
        }
}

SI close(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        return pushres(L, sqlite3_close(ldb->db));
}

SI db_tostring(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        lua_pushfstring(L, "SQLite db: %p", ldb->db);
        return 1;
}

SI db_gc(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        sqlite3_close(ldb->db);
        return 0;
}

SI exec(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        const char *sql = luaL_checklstring(L, 2, NULL);
        char **err;
        int top = lua_gettop(L);
        int (*cb)(void *,int,char**,char**);
        void *arg;
        int res;
        if (top > 2) {
                LERROR("TODO: cb and arg");
        }

        res = sqlite3_exec(ldb->db, sql, cb, arg, err);
        return pushres(L, res);
}

SI get_table(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        const char *sql = luaL_checkstring(L, 2);
        char ***qres;
        int nrow, ncol;
        char **err;
        int row, col, res;
        char *cell;
        res = sqlite3_get_table(ldb->db, sql, qres, &nrow, &ncol, err);

        /* make a table{} and for i=0,nrow add { (i=0,ncol) } */
        lua_createtable(L, nrow, 0);
        printf("nrow=%d, ncol=%d\n", nrow, ncol);
        for (row=0; row <= nrow; row++) {
                lua_createtable(L, ncol, 0);
                for (col=0; col < ncol; col++) {
                        cell = qres[row][col];
                        lua_pushinteger(L, col);
                        if (cell == NULL)
                                lua_pushnil(L);
                        else
                                lua_pushstring(L, cell);
                        lua_settable(L, -3);
                }
                lua_pushinteger(L, row);
                lua_settable(L, -3);
        }

        /* TODO build a Lua matrix w/ results */
        sqlite3_free_table(*qres);
        return pushres(L, res) + 1;
}


/**********/
/* Errors */
/**********/

SI errcode(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        lua_pushinteger(L, sqlite3_errcode(ldb->db));
        return 1;
}

SI errmsg(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        lua_pushstring(L, sqlite3_errmsg(ldb->db));
        return 1;
}


/**************/
/* Statements */
/**************/

SI prepare(lua_State *L) {
        LuaSQLite *ldb = check_db(1);
        size_t len;
        sqlite3_stmt *stmt;
        LuaSQLiteStmt *lstmt;
        const char **tail;
        const char *sql = luaL_checklstring(L, 2, &len);
        int res = sqlite3_prepare_v2(ldb->db, sql, len, &stmt, tail);

        if (res == SQLITE_OK) {
                lstmt = (LuaSQLiteStmt *)lua_newuserdata(L, sizeof(LuaSQLiteStmt *));
                lstmt->stmt = stmt;
                luaL_getmetatable(L, "LuaSQLiteStmt");
                lua_setmetatable(L, -2);
                lua_pushstring(L, *tail);
                return 2;
        } else {
                LERROR(sqlite3_errmsg(ldb->db));
                return 0;
        }
}

SI step(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        return pushres(L, sqlite3_step(s->stmt));
}

SI stmt_gc(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        return pushres(L, sqlite3_finalize(s->stmt));
}

SI reset(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        return pushres(L, sqlite3_reset(s->stmt));
}

SI stmt_tostring(lua_State *L) {
        LuaSQLiteStmt *lstmt = check_stmt(1);
        lua_pushfstring(L, "SQLite stmt: %p", lstmt->stmt);
        return 1;
}


/***********************/
/* Columns and binding */
/***********************/

/*     int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*)); */
/*     int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64); */
/*     int sqlite3_bind_null(sqlite3_stmt*, int); */
/*     int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*)); */
/*     int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*); */
/*     int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n); */

SI bind_param_idx(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        const char *name = luaL_checkstring(L, 2);
        return sqlite3_bind_parameter_index(s->stmt, name);
}

SI param_idx(lua_State *L, sqlite3_stmt *stmt, int i) {
        const char* key;
        if (lua_isnumber(L, i)) {
                return lua_tonumber(L, i);
        } else {
                key = luaL_checkstring(L, i);
                return sqlite3_bind_parameter_index(stmt, key);
        }
}

SI bind_dtype(sqlite3_stmt *s, lua_State *L, int idx) {
        int t;
        const char *v;
        size_t len;

        t = lua_type(L, -1);
        printf("Binding idx %d to type %d\n", idx, t);
        switch (t) {
        case LUA_TNIL:
                return pushres(L, sqlite3_bind_null(s, idx));
        case LUA_TBOOLEAN:
                return pushres(L, sqlite3_bind_int(s, idx, lua_tointeger(L, -1)));
        case LUA_TNUMBER:
                return pushres(L, sqlite3_bind_double(s, idx, lua_tonumber(L, -1)));
        case LUA_TSTRING:
                v = luaL_checklstring(L, -1, &len);
                return pushres(L, sqlite3_bind_text(s, idx, v,
                        len, SQLITE_TRANSIENT));
        default: LERROR("Cannot bind type");
        }
        return 0;
        
}

/* Take a k=v table and call bind(":" .. k, v) for each.
   Take a { v1, v2, v3 } table and call bind([i], v) for each. */
SI bind_table(lua_State *L, LuaSQLiteStmt *s) {
        size_t len = lua_objlen(L, 2);
        int i, idx;
        const char* vartag = ":"; /* TODO make this an optional argument? */

        if (len == 0) {         /* k=v table */
                lua_pushnil(L); /* start at first key */
                while (lua_next(L, 2) != 0) {
                        lua_pushlstring(L, vartag, 1);
                        lua_pushvalue(L, -3);
                        lua_concat(L, 2); /* prepend : to key */
                        idx = sqlite3_bind_parameter_index(s->stmt, lua_tostring(L, -1));
                        if (idx == 0) {
                                lua_pushfstring(L, "Invalid statement parameter '%s'",
                                    lua_tostring(L, -1));
                                lua_error(L);
                        }
                        printf("Index is %d\n", idx);
                        lua_pop(L, 1);
                        bind_dtype(s->stmt, L, idx);
                        lua_pop(L, 1);
                        if (1) printf("* %s (%s) = %s (%s)\n",
                            lua_tostring(L, -2),
                            lua_typename(L, lua_type(L, -2)),
                            lua_tostring(L, -1),
                            lua_typename(L, lua_type(L, -1)));
                        lua_pop(L, 1);
                }
        } else {                /* array */
                for (i=0; i < len; i++) {
                        lua_pushinteger(L, i + 1);
                        lua_gettable(L, 2);
                        bind_dtype(s->stmt, L, i + 1);
                        lua_pop(L, 1);
                }
        }
        return 1;
}

SI bind(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int i;
        if (lua_type(L, 2) == LUA_TTABLE) return bind_table(L, s);

        i = param_idx(L, s->stmt, 2);
        return bind_dtype(s->stmt, L, i);
}

SI bind_param_count(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        return sqlite3_bind_parameter_count(s->stmt);
}

SI bind_double(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int i = param_idx(L, s->stmt, 2);
        double v = luaL_checknumber(L, 3);
        return pushres(L, sqlite3_bind_double(s->stmt, i, v));
}

SI bind_int(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int i = param_idx(L, s->stmt, 2);
        int v = luaL_checkinteger(L, 3);
        return pushres(L, sqlite3_bind_int(s->stmt, i, v));
}

SI bind_null(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int i = param_idx(L, s->stmt, 2);
        return pushres(L, sqlite3_bind_null(s->stmt, i));
}

SI bind_text(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int i = param_idx(L, s->stmt, 2);
        size_t len;
        const char* v = luaL_checklstring(L, 3, &len);
        return pushres(L, sqlite3_bind_text(s->stmt, i, v, len, SQLITE_TRANSIENT));
}

/* const void *sqlite3_column_blob(sqlite3_stmt*, int iCol); */
/* int sqlite3_column_bytes(sqlite3_stmt*, int iCol); */
/* int sqlite3_column_bytes16(sqlite3_stmt*, int iCol); */
/* sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol); */
/* const void *sqlite3_column_text16(sqlite3_stmt*, int iCol); */
/* sqlite3_value *sqlite3_column_value(sqlite3_stmt*, int iCol); */


/* Higher level interface, e.g.
 * id, key, count, score = s:columns("itif") --int, text, int, float */
SI columns(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        size_t len, tlen;
        const char* cs = luaL_checklstring(L, 2, &len);
        double d;
        int i, n;
        const char *t;
        
        int ct = sqlite3_column_count(s->stmt);
        if (len != ct) {
                lua_pushfstring(L, "Invalid column count %d, result has %d columns", len, ct);
                lua_error(L);
        }
        for (i=0; i<len; i++) {
                switch (cs[i]) {
                case 'i':       /* int */
                        n = sqlite3_column_int(s->stmt, i);
                        lua_pushinteger(L, n);
                        break;
                case 'f':       /* float */
                        d = sqlite3_column_double(s->stmt, i);
                        lua_pushnumber(L, d);
                        break;
                case 't':       /* text */
                        tlen = sqlite3_column_bytes(s->stmt, i);
                        t = sqlite3_column_text(s->stmt, i);
                        lua_pushlstring(L, t, tlen);
                        break;
                case 'b':       /* blob */
                        tlen = sqlite3_column_bytes(s->stmt, i);
                        t = (const char*)sqlite3_column_blob(s->stmt, i);
                        lua_pushlstring(L, t, tlen);
                        break;
                default:
                        lua_pushfstring(L, "Invalid column tag '%c' -- must be in 'iftb'",
                            cs[i]);
                        lua_error(L);
                        break;
                }
        }
        
        return len;
}

SI col_double(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int idx = luaL_checkinteger(L, 2);
        double d = sqlite3_column_double(s->stmt, idx - 1);
        lua_pushnumber(L, d);
        return 1;
}

SI col_int(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int idx = luaL_checkinteger(L, 2);
        int i = sqlite3_column_int(s->stmt, idx - 1);
        lua_pushnumber(L, i);
        return 1;
}

SI col_text(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int idx = luaL_checkinteger(L, 2);
        const unsigned char* t = sqlite3_column_text(s->stmt, idx - 1);
        lua_pushstring(L, t);
        return 1;
}

SI col_count(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        lua_pushinteger(L, sqlite3_column_count(s->stmt));
        return 1;
}

SI col_type(lua_State *L) {
        LuaSQLiteStmt *s = check_stmt(1);
        int idx = luaL_checkinteger(L, 2);
        const char *name;
        int t = sqlite3_column_type(s->stmt, idx - 1);
        switch (t) {
        case SQLITE_INTEGER: name = "integer"; break;
        case SQLITE_FLOAT: name = "float"; break;
        case SQLITE_TEXT: name = "text"; break;
        case SQLITE_BLOB: name = "blob"; break;
        case SQLITE_NULL: name = "null"; break;
        default: name = "error"; break;
        }
        lua_pushstring(L, name);
        return 1;
}


/*************/
/* Extension */
/*************/

/*     * sqlite3_create_collation() */
/*     * sqlite3_create_function() */
/*     * sqlite3_create_module() */
/*     * sqlite3_aggregate_context() */
/*     * sqlite3_result() */
/*     * sqlite3_user_data() */
/*     * sqlite3_value() */


/*********/
/* Blobs */
/*********/


/**********/
/* Module */
/**********/

/**************/
/* Metatables */
/**************/

#define LIB(name) static const struct luaL_Reg name[]

/* General database connection. */
/*static const struct luaL_Reg sqlite_mt[] = { */
LIB(db_mt) = {
        { "exec", exec },
        { "get_table", get_table },
        { "errcode", errcode },
        { "errmsg", errmsg },
        { "prepare", prepare },
        { "close", close },
        { "__tostring", db_tostring },
        { "__gc", db_gc },
        { NULL, NULL },
};

LIB(stmt_mt) = {
        { "step", step },
        { "reset", reset },
        { "bind", bind },
        { "bind_double", bind_double },
        { "bind_int", bind_int },
        { "bind_null", bind_null },
        { "bind_text", bind_text },
        { "bind_param_count", bind_param_count },
        { "bind_param_index", bind_param_idx },
        { "column_double", col_double },
        { "column_int", col_int },
        { "column_text", col_text },
        { "column_type", col_type },
        { "column_count", col_count },
        { "columns", columns },
        { "__tostring", stmt_tostring },
        { "__gc", stmt_gc },
        { NULL, NULL },
};

/* Binary blob handle. */
/* ... */


LIB(sqlite_lib) = {
        { "open", open },
        { "version", ver },
        { "version_number", ver_num },
        { NULL, NULL },
};

#define set_MT(mt, t) \
        luaL_newmetatable(L, mt);       \
        lua_pushvalue(L, -1);           \
        lua_setfield(L, -2, "__index"); \
        luaL_register(L, NULL, t);      \
        lua_pop(L, 1)


int luaopen_sqlite(lua_State *L) {
        set_MT("LuaSQLite", db_mt);
        set_MT("LuaSQLiteStmt", stmt_mt);
        luaL_register(L, "sqlite", sqlite_lib);
        return 1;
}
