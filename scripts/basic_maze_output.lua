-- local out = maze("--rows=64 --columns=64 --algo=prims --seed=23 -d -o 64x64_prims_s23.png")
local out = maze("--rows=64 --columns=64 --algo=prims --seed=23 -d")
assert(type(out) == "string" and #out > 0, "maze() returned no output")
print(out)
