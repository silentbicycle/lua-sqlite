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

local s = db:prepare("INSERT INTO foo (t, s) VALUES (:f, :s);")
s:bind(":f", "balloonis")
s:bind(":s", 1234.5)
print("step:", s:step())
print("reset:", s:reset())

s = db:prepare("SELECT * FROM foo;")
print("step:", s:step())
for i=1,3 do print("column ", i, s:column_type(i), s:column_text(i)) end
print("Columns 'it':", s:columns("itf"))
print("reset:", s:reset())



print "BOO"
ron, t = db:get_table("SELECT * FROM foo;")
print "YAH"
print(ron)
my.dump(ron)
print(t)
