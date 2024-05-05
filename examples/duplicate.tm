A: ascii[1..127] = '\u{1}'..'\u{7f}', alt[1..127].
Q: start, copy[1..127], ret_0, ret_1, cleanup.

start:
	ascii{n} -> alt[n], R, copy[n];
	null -> same, N, end.

copy{n}:
	ascii{_} -> same, R, same;
	alt{_} -> same, R, same;
	null -> alt[n], L, ret_0.

ret_0:
	alt{_} -> same, L, same;
	ascii{_} -> same, L, ret_1;
	null -> same, R, cleanup.

ret_1:
	ascii{_} -> same, L, same;
	alt{_} -> same, R, start.

cleanup:
	alt{n} -> ascii[n], R, same;
	null -> same, N, end.