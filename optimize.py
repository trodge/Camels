import sympy as sp
Aa, Pa, Qa, Ra, Ab, Pb, Qb, Rb, Ac, Pc, Qc, Rc, Ad, Pd, Qd, Rd, P = sp.symbols('Aa, Pa, Qa, Ra, Ab, Pb, Qb, Rb, Ac, Pc, Qc, Rc, Ad, Pd, Qd, Rd, P', real = True, positive = True)
f = Pa * Qa + Pb * Qb + Pc * Qc + Pd * Qd - P 
g = (Qa + Aa) / Ra - (Qb + Ab) / Rb
h = (Qb + Ab) / Rb - (Qc + Ac) / Rc
i = (Qc + Ac) / Rc - (Qd + Ad) / Rd
solution = sp.solvers.solve((f, g, h, i), (Qa, Qb, Qc, Qd))
for var in solution:
    print(var, ' = ', sp.simplify(solution[var]))

# Qx = (Rx * (ADP + P - Ax * Px) - Ax * (RDP - Px * Rx)) / RDP
