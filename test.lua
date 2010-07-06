require "lunatest"
require "sqlite"

print("SQLite Version: ", sqlite.version(), sqlite.version_number())


function setup(name)
   db = sqlite.open()
   assert(db:exec([[
CREATE TABLE foo (
   id INTEGER PRIMARY KEY,
   t TEXT NOT NULL,
   s REAL NOT NULL);]]))
end

local function step_and_reset(s)
   assert_equal("done", s:step())
   assert_equal("ok", s:reset())
end


-------------
-- Binding --
-------------

-- Bind by s:bind(":key", value)
function test_bind_imperative()
   local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
   s:bind(":f", "foo")
   s:bind(":s", 1234.5)
   step_and_reset(s)
end

-- Bind by array of values
function test_bind_array()
   local s = db:prepare("INSERT INTO foo (t, s) VALUES (?, ?);")
   s:bind { "abba zabba", 9999.88888 }
   step_and_reset(s)
end

-- Bind by k=v table
function test_bind_table()
   local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
   s:bind { f="banana", s=30949 }
   step_and_reset(s)
end


------------------
-- Getting rows --
------------------

local function add_sample_data_1()
   local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
   for _,row in ipairs{{"foo", 123},
                       {"bar", 456},
                       {"baz", 789}} do
      s:bind { f=row[1], s=row[2] }
      step_and_reset(s)
   end
end

function test_select()
   add_sample_data_1()
   local s = db:prepare("SELECT * FROM foo;")
   while s:step() == "row" do
      local id, t, f = s:columns("itf")
      assert_number(id)
      assert_string(t)
      assert_number(f)
   end   
   assert_equal("ok", s:reset())
end

function test_row_iter_multi()
   add_sample_data_1()
   local s = db:prepare("SELECT * FROM foo;")
   for id, t, score in s:rows("itf") do
      assert_number(id); assert_string(t); assert_number(score)
   end
end

function test_row_iter_list()
   add_sample_data_1()
   local s = db:prepare("SELECT * FROM foo;")
   for row in s:rows("*l") do
      assert_number(row[1]); assert_string(row[2]); assert_number(row[3])
   end
end

function test_row_iter_table()
   add_sample_data_1()
   local s = db:prepare("SELECT * FROM foo;")
   for row in s:rows("*t") do
      assert_number(row.id); assert_string(row.t); assert_number(row.s)
   end
end


---------------
-- Get_table --
---------------

function test_get_table()
   add_sample_data_1()
   local status, res = db:get_table("SELECT * FROM foo;")
   assert_equal("ok", status)
   assert_equal(3, #res)
   -- (sqlite3_get_table returns every value as a string)
   assert_equal("1", res[1][1])
   assert_equal("bar", res[2][2])
   assert_equal("baz", res[3][2])
   assert_equal("789.0", res[3][3])
end


----------------------
-- exec() and hooks --
----------------------

function test_exec1()
   add_sample_data_1()
   local first = true
   local hook = function(colnames, cols)
                   if first then
                      local head = table.concat(colnames, " | ")
                      print("\n" .. head)
                      print(("-"):rep(head:len()))
                      first = false
                   end
                   print(table.concat(cols, " | "))
                end
   assert_equal("ok", db:exec("SELECT * FROM foo;", hook))
   assert_false(first)
end

function test_exec_error()
   add_sample_data_1()
   local first = true
   local hook = function(colnames, cols)
                   error("bananas")
                end
   local ok, err = pcall(function() db:exec("SELECT * FROM foo;", hook) end)
   assert_false(ok, "should catch the error")
   assert_equal("callback requested query abort", err)
end


--------------------
-- Error handling --
--------------------

function test_error_constraint()
   local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
   s:bind { f="foo", s=nil }
   local status, err = s:step()
   assert_false(status)
   assert_equal("constraint", err)
end


-----------------
-- Busy status --
-----------------

function test_busy_timeout()
   assert_equal("ok", db:set_busy_timeout(30))
end

function test_busy_handler()
   -- TODO: make SQLite call the callback
   local status, err = db:set_busy_handler(function(ct) error("AAAAH") end)
   assert_equal("ok", status)
end


lunatest.run()
