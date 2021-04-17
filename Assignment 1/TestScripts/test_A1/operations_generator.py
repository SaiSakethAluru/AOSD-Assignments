import heapq
import random

# ASSUME HEAP SIZE 100

def insert(h, e):
    if len(h) >= 100:
        return -1
    heapq.heappush(h,e)
    return 0


def extractmin(h):
    if len(h) <= 0:
        return -1
    return heapq.heappop(h)


myheap = []

operationsarray = []
resultarray = []

# generate random inserts and extracts
for i in range(1000):
    # 0 = insert, 1 = extractmin
    operation = random.randrange(0, 2)
    if not operation:
        # insert
        element = random.randrange(0, 10000)
        result = insert(myheap, element)
        if not result:
            result = element
        # result is either element insserted, or -1 for error
    
    else:
        result = extractmin(myheap)
    print(operation, result)
    operationsarray.append(operation)
    resultarray.append(result)

# generate many insert
for i in range(101):
    operation = 0
    if not operation:
        # insert
        element = random.randrange(0, 10000)
        result = insert(myheap, element)
        if not result:
            result = element
        # result is either element insserted, or -1 for error
    
    else:
        result = extractmin(myheap)
    print(operation, result)
    operationsarray.append(operation)
    resultarray.append(result)

print(operationsarray)
print(resultarray)