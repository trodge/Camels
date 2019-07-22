import sqlite3
from random import random, sample
from math import sqrt

db = sqlite3.connect("1025ad.db")
c = db.cursor()
'''c.execute("SELECT good_id, material_id FROM consumption ORDER BY good_id, material_id, nation_id")
rs = c.fetchall()
s = set()
for r in rs:
    s.add(r)
rs.clear()
l = list()
for e in s:
    l.append(e)
l.sort()
for e in l:
    c.execute("SELECT nation_id FROM consumption WHERE good_id = ? AND material_id = ?", (e[0], e[1]))
    qs = c.fetchall()
    for n in range(1, 15):
        if not qs.count((n,)):
            c.execute("INSERT INTO consumption VALUES (?, ?, ?, 0, 0, 0)", (n, e[0], e[1]))'''

c.execute("SELECT * FROM consumption ORDER BY nation_id, good_id, material_id")
rs = c.fetchall()
c.execute("DELETE FROM consumption")
for r in rs:
    c.execute("INSERT INTO consumption VALUES (?, ?, ?, ?, ?, ?)", (r[0], r[1], r[2], r[3], r[4], r[5]))
c.execute("SELECT * FROM inputs ORDER BY business_id, mode, good_id")
rs = c.fetchall()
c.execute("DELETE FROM inputs")
for r in rs:
    c.execute("INSERT INTO inputs VALUES (?, ?, ?, ?)", (r[0], r[1], r[2], r[3]))
c.execute("SELECT * FROM outputs ORDER BY business_id, mode, good_id")
rs = c.fetchall()
c.execute("DELETE FROM outputs")
for r in rs:
    c.execute("INSERT INTO outputs VALUES (?, ?, ?, ?)", (r[0], r[1], r[2], r[3]))
c.execute("SELECT * FROM businesses")
rs = c.fetchall()
for r in rs:
    for m in range(1, r[2] + 1):
        c.execute("SELECT nation_id FROM frequencies WHERE business_id = ? AND mode = ?", (r[0], m))
        qs = c.fetchall()
        for n in range(1, 15):
            if not qs.count((n,)):
                c.execute("INSERT INTO frequencies VALUES (?, ?, ?, ?)", (n, r[0], m, 0))
c.execute("SELECT * FROM frequencies ORDER BY nation_id, business_id, mode")
rs = c.fetchall()
c.execute("DELETE FROM frequencies")
for r in rs:
    c.execute("INSERT INTO frequencies VALUES (?, ?, ?, ?)", (r[0], r[1], r[2], r[3]))
c.execute("SELECT * FROM towns WHERE town_id > 14 ORDER BY nation_id, longitude, latitude")
rs = c.fetchall()
c.execute("DELETE FROM towns WHERE town_id > 14")
tid = 14
for r in rs:
    tid += 1
    c.execute("INSERT INTO towns VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", (tid, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9]))
c.execute("SELECT * FROM names ORDER BY nation_id")
rs = c.fetchall()
c.execute("DELETE FROM names")
for r in rs:
    c.execute("INSERT INTO names VALUES (?, ?, ?)", (r[0], r[1], r[2]))
c.execute("SELECT * FROM routes ORDER BY from_id")
rs = c.fetchall()
# Delete duplicate routes
print(rs)
rs = [r for r in rs if r[0] < r[1]]
print('---')
print(rs)
c.execute("DELETE FROM routes")
for r in rs:
    c.execute("INSERT INTO routes VALUES (?, ?)", (r[0], r[1]))
db.commit()
db.close()
