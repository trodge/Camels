import sympy as sp
Pa, Qa, Ra, Pb, Qb, Rb, Pc, Qc, Rc, P = sp.symbols('Pa, Qa, Ra, Pb, Qb, Rb, Pc, Qc, Rc, P', real = True, positive = True)
f = Qa / Ra - Qb / Rb
g = Qb / Rb - Qc / Rc
h = Pa * Qa + Pb * Qb + Pc * Qc - P 
print(sp.solvers.solve((f, g, h), (Qa, Qb, Qc)))