require "sqlite"

bob = sqlite.open()

print("Version: ", sqlite.version())
print("Version #: ", sqlite.version_number())

print("db is ", bob)

joe = bob:prepare("SELECT * FROM sqlite_master;")
print(joe)
print(joe:step())


joe = bob:exec([[
CREATE TABLE foo (
   id INTEGER PRIMARY KEY,
   foo TEXT);]])

print(joe)

print "BOO"
ron, t = bob:get_table("SELECT * FROM sqlite_master;")
print "YAH"
print(ron)
print(t)
