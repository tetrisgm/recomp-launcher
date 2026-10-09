#!/usr/bin/env python3
"""gen_supported.py [--check]: rebuild the capability table in docs/SUPPORTED.md
from the catalog in src/r4l/core/surface.cpp (between the BEGIN/END markers)."""
import re, sys, os
root = os.path.join(os.path.dirname(__file__), "..")
src = open(os.path.join(root, "src/r4l/core/surface.cpp")).read()
rows = re.findall(r'\{"([a-z_.]+)", (true|false), "(\w+)", "([^"]+)", ([^}]+)\}', src)
lines = ["| Key | Area | What it is | Default | Settings field |", "|---|---|---|---|---|"]
for key, ess, area, label, off in rows:
    field = re.search(r"S_OFF\((\w+)\)", off)
    lines.append("| `%s` | %s | %s | %s | %s |" % (key, area, label, "Shown" if ess == "true" else "Hidden",
                                                    "`%s`" % field.group(1) if field else "-"))
table = "\n".join(lines)
p = os.path.join(root, "docs/SUPPORTED.md")
doc = open(p).read()
new = re.sub(r"<!-- BEGIN CAPABILITIES -->.*<!-- END CAPABILITIES -->",
             "<!-- BEGIN CAPABILITIES -->\n" + table + "\n<!-- END CAPABILITIES -->", doc, flags=re.S)
if "--check" in sys.argv:
    if new != doc:
        print("docs/SUPPORTED.md is out of date: run tools/gen_supported.py"); sys.exit(1)
    print("SUPPORTED.md current (%d capabilities)" % len(rows)); sys.exit(0)
open(p, "w").write(new)
print("wrote %d capabilities" % len(rows))
