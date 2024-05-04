A: dec[0..9] = '0'..'9', true, false.
Q: q[0..2], q[0] = start.

q[0]:
	null -> true, N, end.

q{r}:
	dec{d} -> same, R, q[(d + r) % 3];
	null -> false, N, end.