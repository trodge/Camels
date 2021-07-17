import sympy as sp
Aa, Ia, Pa, Ra, Sa, Ab, Ib, Pb, Rb, Sb, P = sp.symbols('Aa, Ia, Pa, Ra, Sa, Ab, Ib, Pb, Rb, Sb, P', real = True, positive = True)
f = (Aa - (Ia - sp.sqrt((Ia - Sa * Aa) ** 2 + Sa * Pa * 2)) / Sa) * Rb - (Ab - (Ib - sp.sqrt((Ib - Sb * Ab) ** 2 + Sb * Pb * 2)) / Sb) * Ra
g = Pa + Pb - P
for s in sp.solve((f, g), (Pa, Pb)):
    print(sp.factor(s), '\n')
