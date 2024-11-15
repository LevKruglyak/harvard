library(tidyverse)
vowels2 = read.csv("vowels2.txt", sep="\t")
words_trimmed = read.csv("words_trimmed.txt", sep="\t")

# Problem 1
vowel_i <- vowels2 |> filter(vowel == "i")
plot <- ggplot(vowel_i, aes(x = duration)) +
  geom_density(fill = "blue", alpha = 0.5) +
  labs(title = "Density of duration for the vowel [i]",
       x = "Duration",
       y = "Density")
ggsave("problem1.png", plot = plot, width = 6, height = 4, dpi = 300)

print(vowel_i |> summarize(mean_duration = mean(duration, na.rm = TRUE)))
print(vowel_i |> summarize(median_duration = median(duration, na.rm = TRUE)))

# Problem 2
plot2 <- ggplot(vowels2, aes(x = vowel, y = duration)) +
  geom_violin(draw_quantiles = 0.5, fill = "blue", alpha = 0.5) +
  labs(title = "Violin Plot of Vowel Durations",
       x = "Vowel",
       y = "Duration") +
  theme_minimal()
ggsave("problem2.png", plot = plot2, width = 6, height = 4, dpi = 300)

vowel_averages <- vowels2 |> 
  group_by(vowel) |> 
  summarize(avg_duration = mean(duration, na.rm = TRUE)) |> 
  arrange(avg_duration)
print(vowel_averages)

# Problem 3
vowel_ie_speaker4 <- vowels2 |>
  filter(vowel %in% c("i", "e") & speaker == 4)

median_durations <- vowel_ie_speaker4 |>
  group_by(vowel) |>
  summarize(median_duration = median(duration, na.rm = TRUE))
print(median_durations)
wilcox_test <- wilcox.test(duration ~ vowel, data = vowel_ie_speaker4)
print(paste("W statistic:", wilcox_test$statistic))
print(paste("p-value:", wilcox_test$p.value))
print(ifelse(wilcox_test$p.value < 0.05, "The difference is significant.", "The difference is not significant."))

# Problem 4
plot4 <- ggplot(vowel_i, aes(x = freq, y = duration)) +
  geom_point(alpha = 0.6) +
  geom_smooth(method = "loess", color = "blue", se = TRUE) +
  labs(title = "Scatter plot of duration vs. frequency for Vowel [i]",
       x = "Word Frequency",
       y = "Vowel Duration") +
  theme_minimal()
print(plot4)
ggsave("problem4.png", plot = plot4, width = 6, height = 4, dpi = 300)

# Problem 5
num_two_nasals <- words_trimmed |>
  filter(str_detect(Surface, "[mMnN]{2,}")) |>
  nrow()
print(paste("Number of words with two or more nasals in a row:", num_two_nasals))
num_begin_end_nasal <- words_trimmed |>
  filter(str_detect(Surface, "^[mMnN].*[mMnN]$")) |>
  nrow()
print(paste("Number of words that both begin and end with a nasal:", num_begin_end_nasal))

# Problem 6
what <- words_trimmed |> filter(Word == "what") |> mutate(t_deletes = !str_detect(Surface, "t$"))
deletion_percentage <- what |>
  summarize(deletion_rate = mean(t_deletes) * 100) |>
  pull(deletion_rate)
print(paste("Percentage of time 't' is deleted in the word 'what':", deletion_percentage, "%"))

# Problem 7
deletion_by_speaker <- what |>
  group_by(Speaker) |>
  summarize(deletion_rate = mean(t_deletes) * 100) |>
  arrange(desc(deletion_rate))

highest_deletion <- deletion_by_speaker |> slice(1)
lowest_deletion <- deletion_by_speaker |> slice(n())

print("Speaker with the highest deletion rate:")
print(highest_deletion)

print("Speaker with the lowest deletion rate:")
print(lowest_deletion)

# Problem 8

deletion_by_speaker <- what |>
  group_by(Speaker, Gender, Age) |>
  summarize(deletion_rate = mean(t_deletes) * 100, .groups = "drop")

plot8 <- ggplot(deletion_by_speaker, aes(x = Gender, y = deletion_rate)) +
  geom_boxplot() +
  facet_wrap(~ Age) +
  labs(title = "Deletion rate by gender and age",
       x = "Gender",
       y = "Rate of Deletion (%)") +
  theme_minimal()

# Save the plot
ggsave("problem8.png", plot = plot8, width = 8, height = 6, dpi = 300)