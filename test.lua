require "sqlite"

print("Version: ", sqlite.version())
print("Version #: ", sqlite.version_number())

db = sqlite.open()
print(db)

print("making table:", db:exec([[
CREATE TABLE foo (
   id INTEGER PRIMARY KEY,
   t TEXT);]]))

local s = db:prepare("INSERT INTO foo (t) VALUES (:f);")
s:bind(":f", "balloonis")
print("step:", s:step())
print("reset:", s:reset())

s = db:prepare("SELECT * FROM foo;")
print("step:", s:step())
print(1, s:col_type(1), s:col_text(1))
print("reset:", s:reset())


print "BOO"
ron, t = db:get_table("SELECT * FROM foo;")
print "YAH"
print(ron)
my.dump(ron)
print(t)
