N, T = (int(i) for i in raw_input().split())
people = [raw_input() for j in range(N)]

max_topics = 0
max_teams = 0

for i, person1 in enumerate(people):
    for j, person2 in enumerate(people[i+1:]):
        binary1 = int(person1, 2)
        binary2 = int(person2, 2)
        topics = bin(binary1 | binary2).count('1')
                
        if topics > max_topics:
            max_topics = topics
            max_teams = 1
        elif topics == max_topics:
            max_teams += 1
            
print max_topics
print max_teams