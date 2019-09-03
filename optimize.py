import sympy as sp
Aa, Ba, Ia, Sa, Qa, Ra, Ab, Bb, Ib, Sb, Qb, Rb, Ac, Bc, Ic, Sc, Qc, Rc, P = sp.symbols('Aa,Ba,Ia,Sa,Qa,Ra,Ab,Bb,Ib,Sb,Qb,Rb,Ac,Bc,Ic,Sc,Qc,Rc,P', real = True)
f = Qa / Ra - Qb / Rb
g = Qb / Rb - Qc / Rc
h = (Ia - Sa * (Aa - Qa / 2)) * Qa + (Ib - Sb * (Ab - Qb / 2)) * Qb + (Ic - Sc * (Ac - Qc / 2)) * Qc - P
for s in sp.solvers.solve((f, g, h), (Qa, Qb, Qc)):
    print('Qa = ', s[0])
    print('Qb = ', s[1])
    print('Qc = ', s[2])