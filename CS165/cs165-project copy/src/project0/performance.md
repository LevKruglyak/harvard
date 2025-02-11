# Chained Hash Table (Load factor 0.9)
Memory overhead for 4096 entries:       150K
Time to do 50,000,000 insertions:       3.4s
Time to do 50,000,000 insertions (d):   13.3s
Time to do 50,000,000 reads (up to 5):  5.1s
Time to do 50,000,000 deletions:        7.2s

# Robinhood Hash Table (Load factor 0.5)
Memory overhead for 4096 entries: 100K
  -> at the same load factor as chained uses 1/3 of the memory
Time to do 50,000,000 insertions:       1.8s
Time to do 50,000,000 insertions (d):   2.3s
Time to do 50,000,000 reads (up to 5):  3.9s
Time to do 50,000,000 deletions:        3.2s
