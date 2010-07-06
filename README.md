(Yet Another*) Lua wrapper for SQLite wrapper.

For example usage, see the test suite. Documentation (and more tests) to
come. See also the TODO.

Basic operation:

    -- Connect to the database
    db = sqlite.open()     -- defaults to in-memory

    -- exec
    assert(db:exec([[
    CREATE TABLE foo (
    id INTEGER PRIMARY KEY,
    t TEXT NOT NULL,
    s REAL NOT NULL);]]))

    -- compiled statements
    local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")

    -- binding
    s:bind(":f", "foo")
    -- OR
    s:bind { f="foo", s="bar" } --the : is appended automatically
    -- OR
    s:bind { "abba zabba", 9999.88888 } -- (for anonymous ? variables)
    s:step()
    s:reset()

    -- results
    local s = db:prepare("SELECT * FROM foo;")
    -- multi-result-style
    for id, t, score in s:rows("itf") do ... -- "itf" = "as int, text, & float"
    -- list of results
    for row in s:rows("*l") do ...
    -- { column_nameN = column_valueN, ... } table for each row
    for row in s:rows("*t") do

Since Lua doesn't use string encoding, I'm using the UTF-8 version of
SQLite's API throughout. If you need more specific support, Lua's "raw
byte-array and a length"-style strings will work with a Unicode library.

*Of the two existing, one is moribund (2006) and isn't compatible with
the current version of Lua, and the other has some quirks I don't
care for.
