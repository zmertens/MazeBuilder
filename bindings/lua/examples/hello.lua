local out = maze("--rows=5 --columns=5 --algo=dfs --seed=1")
assert(type(out) == "string" and #out > 0, "maze() returned no output")
print(out)
