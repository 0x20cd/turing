# Ввод: целое неотрицательное десятичное число. Каретка указывает на первый символ.
# Вывод: двоичное представление числа.

A: dec[0..9] = '0'..'9', bin[0..1] = '01'.
Q: start, q[0..1], skip_L[0..1], skip_R, w[0..1], cleanup.

start:
	dec[0] -> same, R, same;
	dec{_} -> same, N, q[0];
	null -> same, L, cleanup.

q[0]:
	dec{n} -> dec[n / 2], R, q[n % 2].

q[1]:
	dec{n} -> dec[(10 + n) / 2], R, q[n % 2].

q{i}:
	null -> same, L, skip_L[i].

skip_L{i}:
	dec{_} -> same, L, same;
	null -> same, L, w[i].

w{i}:
	bin{_} -> same, L, same;
	null -> bin[i], R, skip_R.

skip_R:
	bin{_} -> same, R, same;
	null -> same, R, start.

cleanup:
	dec[0] -> null, L, same;
	null -> same, L, end.