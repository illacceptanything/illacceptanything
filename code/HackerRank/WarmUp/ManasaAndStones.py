T = int(input())

for i in range(T):
    n = int(input())
    a = int(input())
    b = int(input())
    values = sorted(set([(n - i - 1) * a + i * b for i in range(n)]))
    print(*values) 