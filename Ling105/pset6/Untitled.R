library(tidyverse)
x = read.csv("words12.txt", sep="\t")

# Problem 1
print(x |> filter(str_detect(Word, "aught$")) |> select(Speaker, Word, Surface))
print(x |> filter(str_detect(Word, "ot$")) |> select(Speaker, Word, Surface))

# Problem 2
print(x |> filter(Word == "probably") |> select(SylNSurface) |> pull() |> table())

# Problem 3
print(x |> group_by(Speaker) |> summarize(avg_duration = mean(Duration, na.rm = TRUE)))

# Problem 4
print(x |> filter(Word == "the") |> summarize(avg_duration = median(Duration, na.rm = TRUE)))

# Problem 5
print(x |> filter(str_detect(Word, "th$")) |> summarize(min_duration = min(Duration), max_duration = max(Duration)))

# Problem 6
print(x |> filter(str_detect(POS, "^N")) |> summarize(noun_mean_syllables = mean(SylNSurface)))
print(x |> filter(str_detect(POS, "^V")) |> summarize(verb_mean_syllables = mean(SylNSurface)))

# Problem 7
print(x |> group_by(Word) |> summarize(n = n()) |> arrange(desc(n)))

# Problem 8
print(x |> filter(POS == "VBN") |> group_by(Word) |> summarize(n = n()) |> arrange(desc(n)))

# Problem 9
print(x |> group_by(POS) |> summarize(n = n(), frequency = n / nrow(x) * 100) |> arrange(desc(n)))

# Problem 10
print(x |> group_by(SylNSurface) |> summarize(avg_syl_length = mean(Duration) / unique(SylNSurface)))