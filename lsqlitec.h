#ifndef LSQLITE_H
#define LSQLITE_H

typedef struct LuaSQLite {
        sqlite3 *db;
} LuaSQLite;


typedef struct LuaSQLiteStmt {
        sqlite3_stmt *stmt;
} LuaSQLiteStmt;


#define check_db(n)                                   \
        ((LuaSQLite *)luaL_checkudata(L, n, "LuaSQLite"))
#define check_stmt(n)                                   \
        ((LuaSQLiteStmt *)luaL_checkudata(L, n, "LuaSQLiteStmt"))


#define LERROR(s) \
        lua_pushstring(L, s);                   \
        lua_error(L);

#endif
