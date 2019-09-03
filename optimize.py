import sympy as sp
Aa, Ba, Ia, Sa, Qa, Ra, Ab, Bb, Ib, Sb, Qb, Rb, Ac, Bc, Ic, Sc, Qc, Rc, Ad, Bd, Id, Sd, Qd, Rd, P = sp.symbols('Aa,Ba,Ia,Sa,Qa,Ra,Ab,Bb,Ib,Sb,Qb,Rb,Ac,Bc,Ic,Sc,Qc,Rc,Ad,Bd,Id,Sd,Qd,Rd,P', real = True, positive = True)
f = Qa / Ra - Qb / Rb
g = Qb / Rb - Qc / Rc
h = Qc / Rc - Qd / Rd
i = (Ia - Sa * (Aa - Qa / 2)) * Qa + (Ib - Sb * (Ab - Qb / 2)) * Qb + (Ic - Sc * (Ac - Qc / 2)) * Qc + (Id - Sd * (Ad - Qd / 2)) * Qd - P
for s in sp.solvers.solve((f, g, h, i), (Qa, Qb, Qc, Qd)):
    print('Qa = ', sp.factor(s[0]))
    print('Qb = ', sp.factor(s[1]))
    print('Qc = ', sp.factor(s[2]))
    print('Qd = ', sp.factor(s[3]))