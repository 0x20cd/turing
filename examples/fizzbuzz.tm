# Ввод: пустая лента.
# Вывод: числа от 1 до 1000, разделенные пробелом. При этом числа, делящиеся на 3 заменены
# на "Fizz", делящиеся на 5 заменены на "Buzz", а делящиеся на 15 - на "FizzBuzz".

A: null = ' ', fb[0..1][0..3] = "FizzBuzz", dec[0..9] = '0'..'9', cmd[0..3].
Q: main[1..1001], main[1] = start, main[1001] = end, prnum[1..1000][0..4], prfb[1..1000][1..3][0..4].

main{n}:
	null -> cmd[(3 - n % 3) / 3 + (5 - n % 5) / 5 * 2], N, same;
	cmd{c | 1..3} -> null, N, prfb[n][c][0].

main{n | 1..9}:     cmd[0] -> null, N, prnum[n][1].
main{n | 10..99}:   cmd[0] -> null, N, prnum[n][2].
main{n | 100..999}: cmd[0] -> null, N, prnum[n][3].
main{n | 1000}:     cmd[0] -> null, N, prnum[n][4].

prnum{n}[0]: null -> null, R, main[n + 1].
prnum{n}{k}: null -> dec[n / 10 ^ (k - 1) % 10], R, prnum[n][k - 1].

prfb{n}[3][4]: null -> null, N, prfb[n][2][0].
prfb{n}{c}[4]: null -> null, R, main[n + 1].
prfb{n}{c}{i}: null -> fb[(c + 1) % 2][i], R, prfb[n][c][i + 1].
