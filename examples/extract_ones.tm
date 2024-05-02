A: null = ' ', b[0..1] = '01', bridge.
Q: start, to_w1, w1, ret.

start:
	b[0] -> same, R, same;
	b[1] -> b[0], R, to_w1;
	null -> same, N, end.

to_w1:
	b{_} -> same, R, same;
	null -> bridge, R, w1.

w1:
	b[1] -> same, R, same;
	null -> b[1], L, ret.

ret:
	bridge -> null, L, same;
	b{_} -> same, L, same;
	null -> same, R, start.