import sympy as sp
Pa, Qa, Ra, Pb, Qb, Rb, Pc, Qc, Rc, P = sp.symbols('Pa, Qa, Ra, Pb, Qb, Rb, Pc, Qc, Rc, P', real = True, positive = True)
f = Pa * Qa + Pb * Qb + Pc * Qc - P 
g = Qa / Ra - Qb / Rb
h = Qb / Rb - Qc / Rc
print(sp.solvers.solve((f, g, h), (Qa, Qb, Qc)))