A: hex[0..15] = '0'..'9' & 'a' .. 'f', leg[-1..1] = "<=>".
Q: start, cmp[0..15], res[-1..1].

start:
	hex{x} -> same, R, cmp[x].

cmp{x}:
	hex[x] -> same, R, res[0].

cmp{x | 1..15}:
	hex{y | 0..x-1} -> same, R, res[1].

cmp{x | 0..14}:
	hex{y | x+1..15} -> same, R, res[-1].

res{r}:
	null -> leg[r], N, end.