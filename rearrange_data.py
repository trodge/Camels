import sqlite3
from random import random, sample
from math import sqrt

db = sqlite3.connect("1025ad.db")
c = db.cursor()
# Sort consumption.
c.execute("SELECT * FROM consumption ORDER BY nation_id, good_id, material_id")
rs = c.fetchall()
c.execute("DELETE FROM consumption")
for r in rs:
    c.execute("INSERT INTO consumption VALUES (?, ?, ?, ?, ?, ?)", (r[0], r[1], r[2], r[3], r[4], r[5]))
# Sort inputs and outputs.
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
# Put zeros in frequencies.
c.execute("SELECT * FROM businesses")
rs = c.fetchall()
for r in rs:
    for m in range(1, r[2] + 1):
        c.execute("SELECT nation_id FROM frequencies WHERE business_id = ? AND mode = ?", (r[0], m))
        qs = c.fetchall()
        for n in range(1, 15):
            if not qs.count((n,)):
                c.execute("INSERT INTO frequencies VALUES (?, ?, ?, ?)", (n, r[0], m, 0))
# Sort frequencies.
c.execute("SELECT * FROM frequencies ORDER BY nation_id, business_id, mode")
rs = c.fetchall()
c.execute("DELETE FROM frequencies")
for r in rs:
    c.execute("INSERT INTO frequencies VALUES (?, ?, ?, ?)", (r[0], r[1], r[2], r[3]))
# Sort and re-index towns.
c.execute("SELECT * FROM towns WHERE town_id > 14 ORDER BY nation_id, longitude, latitude")
rs = c.fetchall()
c.execute("DELETE FROM towns WHERE town_id > 14")
tid = 14
for r in rs:
    tid += 1
    c.execute("INSERT INTO towns VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", (tid, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9]))
# Sort names.
c.execute("SELECT * FROM names ORDER BY nation_id")
rs = c.fetchall()
c.execute("DELETE FROM names")
for r in rs:
    c.execute("INSERT INTO names VALUES (?, ?, ?)", (r[0], r[1], r[2]))
# Adjust livestock outputs for daily counting.
c.execute('SELECT amount, business_id, mode, good_id FROM outputs WHERE 58 < good_id AND good_id < 66 AND business_id = 4')
ns = c.fetchall()
c.execute('SELECT amount FROM inputs WHERE 58 < good_id AND good_id < 66 AND business_id = 4')
ms = c.fetchall();
for m, n in zip(ms, ns):
    print(m, n)
    print(m[0] * (n[0] / m[0]) ** (1 / 365))
    c.execute('UPDATE outputs SET amount = ? WHERE business_id = ? AND mode = ? AND good_id = ?', (m[0] * (n[0] / m[0]) ** (1 / 365), n[1], n[2], n[3]))
db.commit()
db.close()
