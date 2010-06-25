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

-- Bind by s:bind(":key", value)
local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
s:bind(":f", "balloonis")
s:bind(":s", 1234.5)
print("step:", s:step())
print("reset:", s:reset())

-- Bind by array of values
s = db:prepare("INSERT INTO foo (t, s) VALUES (?, ?);")
s:bind { "abba zabba", 9999.88888 }
print("step:", s:step())
print("reset:", s:reset())

-- Bind by k=v table
s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
s:bind { f="malarkey", s=30949 }
print("step:", s:step())
print("reset:", s:reset())


s = db:prepare("SELECT * FROM foo;")
while s:step() == "row" do
   --for i=1,3 do print("column ", i, s:column_type(i), s:column_text(i)) end
   print("Columns 'it':", s:columns("itf"))
   
end
print("reset:", s:reset())


-- TODO implicitly wrap s:step(), checking result, and s:reset()
if false then
   for id, text, score in s:column_iter("itf") do
      print(id, text, score)
   end
end

ok, res = db:get_table("SELECT * FROM foo;")
print("Status is ", ok)
print("ok", ok, ok == "ok")
if ok == "o" .. "k" then my.dump(res) end


