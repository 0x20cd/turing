# Ввод: два целых неотрицательных десятичных числа, разделённых пробелом.
# Каретка указывает на первый символ.
# Вывод: результат сложения чисел.

A: null, d[0..1][0..9] = '0'..'9' & '𝟘'..'𝟡', space = ' '.
Q: start, to_grab, grab, carry[0..9], to_add[0..9], add[0..9], to_clean, cleanup_1, cleanup_2.


start:
	d{_}{_} -> same, R, same;
	space -> same, R, to_grab.


to_grab:
	d[0]{_} -> same, R, same;
	null -> same, L, grab;
	d[1]{_} -> same, L, grab.

grab:
	d[0]{n} -> d[1][n], L, carry[n];
	space -> same, R, to_clean.


carry{c}:
	d{_}{_} -> same, L, same;
	space -> same, L, to_add[c].


to_add{c}:
	d[1]{_} -> same, L, same;
	d[0]{n} -> d[1][n], N, add[c];
	null -> d[1][c], R, start.

add[0]:
	d{_}{_} -> same, R, start;
	null -> null, R, start.

add{c | 1..9}:
	d{x}{n | 0..9-c} -> d[x][n + c], R, start;
	d{x}{n | 10-c..9} -> d[x][(n + c) % 10], L, add[1];
	null -> d[0][c], R, start.


to_clean:
	d{_}{_} -> same, R, same;
	null -> same, L, cleanup_1.

cleanup_1:
	d[1]{_} -> null, L, same;
	space -> null, L, cleanup_2.

cleanup_2:
	d[1]{n} -> d[0][n], L, same;
	d[0]{_} -> same, L, same;
	null -> same, R, end.