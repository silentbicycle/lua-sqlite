require "sqlite"

print("Version: ", sqlite.version())
print("Version #: ", sqlite.version_number())

db = sqlite.open()
print(db)

print("making table:", db:exec([[
CREATE TABLE foo (
   id INTEGER PRIMARY KEY,
   t TEXT NOT NULL,
   s REAL NOT NULL);]]))

print "ok"

-- Bind by s:bind(":key", value)
local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
s:bind(":f", "balloonis")
s:bind(":s", 1234.5)
print "ok"
print("step:", s:step())
print("reset:", s:reset())

-- Bind by array of values
s = db:prepare("INSERT INTO foo (t, s) VALUES (?, ?);")
s:bind { "abba zabba", 9999.88888 }
print("step:", s:step())
print("reset:", s:reset())

print "ok"

-- Bind by k=v table
s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
s:bind { f="malarkey", s=30949 }
print("step:", s:step())
print("reset:", s:reset())

print "ok"

s = db:prepare("SELECT * FROM foo;")
while s:step() == "row" do
   --for i=1,3 do print("column ", i, s:column_type(i), s:column_text(i)) end
   print("Columns 'it':", s:columns("itf"))
   
end
print("reset:", s:reset())

print "ok"

-- FIXME add this to the C lib
local mt = getmetatable(s)
mt.column_iter =
   function(self, tag)
      return function()
                local status = s:step()
                if status == "row" then
                   return s:columns(tag)
                elseif status == "done" then
                   s:reset()
                   return nil
                else
                   s:reset()
                   error(sqlite3_errmsg(db))
                end
             end
   end


if true then
   print("----------------------------------------")
   for id, text, score in s:rows("itf") do
      print(id, text, score)
   end
   print("----------------------------------------")
end

print "ok"

print "GET_TABLE test"
ok, res = db:get_table("SELECT * FROM foo;")
print("Status is ", ok)
print("ok", ok, ok == "ok")
if ok == "o" .. "k" then my.dump(res) end
my.dump(res)

print "ok"

print "EXEC test"
local first = true
local hook = function(colnames, cols)
                if false and first then
                   local head = table.concat(colnames, " | ")
                   print(head)
                   print(("-"):rep(head:len()))
                   first = false
                end
                --printf(table.concat(cols, " | "))  -- uncomment for an error
                print(table.concat(cols, " | "))
             end
if true then
   print("EXEC", db:exec("SELECT * FROM foo;", hook))
end

print "DONE"
