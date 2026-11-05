local out = maze("--rows=5 --columns=7 --algo=dfs --seed=12 -d")
assert(type(out) == "string" and #out > 0, "maze() returned no output")
print(out)
