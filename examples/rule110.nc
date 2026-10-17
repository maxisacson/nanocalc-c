N = 512
M = 128
state = [0..0..M]
state[M-1] = 1

f(i) = {
    0 if state[i-1..i+1] == [1, 1, 1]
    1 if state[i-1..i+1] == [1, 1, 0]
    1 if state[i-1..i+1] == [1, 0, 1]
    0 if state[i-1..i+1] == [1, 0, 0]
    1 if state[i-1..i+1] == [0, 1, 1]
    1 if state[i-1..i+1] == [0, 1, 0]
    1 if state[i-1..i+1] == [0, 0, 1]
    0 if state[i-1..i+1] == [0, 0, 0]
}

put state
print
for i in 1..N-1 {
    next = [0..0..M]
    for j in 0..#state-1 {
        next[j] = f(j)
    }
    state = next
    put state
    print
}
