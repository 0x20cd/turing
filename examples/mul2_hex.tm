A: hex[0..15] = '0'..'9' & 'a'..'f'.
Q: start, mul[0..1].

start:
	hex{_} -> same, R, same;
	null -> same, L, mul[0].

mul{c}:
	hex{n} -> hex[(2 * n + c) % 16], L, mul[(2 * n + c) / 16].

mul[0]: null -> null, R, end.
mul[1]: null -> hex[1], N, end.
