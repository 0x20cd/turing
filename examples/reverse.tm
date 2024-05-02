A: null, text[1..193] = '\u{1}'..'\u{7f}' & 'а'..'я' & 'А'..'Я' & 'ё' & 'Ё', bridge, erased.
Q: start, mem, q[1..193], w[1..193], ret, ifstart, cleanup.

start:
	text{_} -> same, R, same;
	erased -> same, L, mem;
	null -> same, L, mem.

mem:
	null -> same, N, end;
	text{n} -> erased, R, q[n].

q{n}:
	text{_} -> same, R, same;
	null -> bridge, R, w[n];
	erased -> same, R, same.

w{n}:
	text{_} -> same, R, same;
	null -> text[n], L, ret.

ret:
	text{_} -> same, L, same;
	bridge -> null, L, same;
	erased -> same, L, same;
	null -> same, R, ifstart.

ifstart:
	erased -> same, N, cleanup;
	text{_} -> same, N, start.

cleanup:
	erased -> null, R, same;
	null -> same, R, end.