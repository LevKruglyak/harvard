import numpy as np
import numpy.random as npr
import pygame as pg

# uncomment this for animation
from p3src.SwingyMonkey import SwingyMonkey as SwingyMonkeyAnimated

# uncomment this for no animation (use this for most purposes! it gets very slow otherwise)
from p3src.SwingyMonkeyNoAnimation import SwingyMonkey

# Some constants. Don't edit this!
X_BINSIZE = 200
Y_BINSIZE = 100
X_SCREEN = 1400
Y_SCREEN = 900


class RandomJumper(object):
    """
    This agent jumps randomly.
    """

    def __init__(self):
        self.last_state = None
        self.last_action = None
        self.last_reward = None

        # We initialize our Q-value grid that has an entry for each action and state.
        # (action, rel_x, rel_y)
        self.Q = np.zeros((2, X_SCREEN // X_BINSIZE, Y_SCREEN // Y_BINSIZE))

    def reset(self):
        self.last_state = None
        self.last_action = None
        self.last_reward = None

    def discretize_state(self, state):
        """
        Discretize the position space to produce binned features.
        rel_x = the binned relative horizontal distance between the monkey and the tree
        rel_y = the binned relative vertical distance between the monkey and the tree
        """

        rel_x = int((state["tree"]["dist"]) // X_BINSIZE)
        rel_y = int((state["tree"]["top"] - state["monkey"]["top"]) // Y_BINSIZE)
        return (rel_x, rel_y)

    def action_callback(self, state):
        """
        Implement this function to learn things and take actions.
        Return 0 if you don't want to jump and 1 if you do.
        """

        new_action = npr.rand() < 0.1
        new_state = state

        self.last_action = new_action
        self.last_state = new_state

        return self.last_action

    def reward_callback(self, reward):
        """This gets called so you can see what reward you get."""

        self.last_reward = reward


class Learner(object):
    def __init__(self, alpha=0.1, gamma=0.95, epsilon=0.01):
        self.alpha = alpha
        self.gamma = gamma
        self.epsilon = epsilon
        self.Q = {}
        self.last_state = None
        self.last_action = None

    def discretize_state(self, state):
        rel_x = int((state["tree"]["dist"]) // X_BINSIZE)
        rel_y = int((state["tree"]["top"] - state["monkey"]["top"]) // Y_BINSIZE)
        return (rel_x, rel_y)

    def action_callback(self, state):
        dstate = self.discretize_state(state)
        if dstate not in self.Q:
            self.Q[dstate] = [0, 0]

        if np.random.rand() < self.epsilon:
            action = np.random.choice([0, 1])
        else:
            action = np.argmax(self.Q[dstate])

        if self.last_state is not None:
            reward = self.reward
            best_future = max(self.Q.get(dstate, [0, 0]))

            self.Q[self.last_state][self.last_action] += self.alpha * (
                reward
                + self.gamma * best_future
                - self.Q[self.last_state][self.last_action]
            )

        self.last_state = dstate
        self.last_action = action
        return action

    def reward_callback(self, reward):
        self.reward = reward

    def reset(self):
        self.last_state = None
        self.last_action = None
        self.last_reward = None


class LearnerOptimized(object):
    def __init__(self, alpha=0.5, gamma=0.95, epsilon=0.01):
        self.alpha = alpha
        self.gamma = gamma
        self.epsilon = epsilon
        self.Q = {}
        self.reset()

    def discretize_state(self, state):
        rel_x = int((state["tree"]["dist"]) // X_BINSIZE)
        rel_y = int((state["tree"]["top"] - state["monkey"]["top"]) // Y_BINSIZE)
        vel_y = int(
            (state["monkey"]["vel"] * state["tree"]["dist"]) // (X_BINSIZE * Y_BINSIZE)
        )
        return (rel_x, rel_y, vel_y)

    def action_callback(self, state):
        dstate = self.discretize_state(state)
        if dstate not in self.Q:
            self.Q[dstate] = [0, 0]

        if np.random.rand() < self.epsilon / (state["score"] + 1):
            action = np.random.choice([0, 1])
        else:
            action = np.argmax(self.Q[dstate])

        if self.last_state is not None:
            reward = self.reward
            best_future = max(self.Q[dstate])

            self.Q[self.last_state][self.last_action] += self.alpha * (
                reward
                + self.gamma * best_future
                - self.Q[self.last_state][self.last_action]
            )

        self.last_state = dstate
        self.last_action = action

        return action

    def reward_callback(self, reward):
        self.reward = reward

    def reset(self):
        self.last_state = None
        self.last_action = None
        self.last_reward = None


def run_games(learner, hist, iters=100, t_len=100):
    """
    Driver function to simulate learning by having the agent play a sequence of games.
    """
    for ii in range(iters):
        # Make a new monkey object.
        swing = SwingyMonkey(
            sound=False,  # Don't play sounds.
            text="Epoch %d" % (ii),  # Display the epoch on screen.
            tick_length=t_len,  # Make game ticks super fast.
            action_callback=learner.action_callback,
            reward_callback=learner.reward_callback,
        )

        # Loop until you hit something.
        while swing.game_loop():
            pass

        # Save score history.
        hist.append(swing.score)

        # Reset the state of the learner.
        learner.reset()
    pg.quit()
    return


# max_learner = []
#
# for i in range(0, 100):
#     agent = Learner()
#     hist = []
#     run_games(agent, hist, 100, 100)
#     max_learner.append(max(hist))
#
# print("basic learner:")
#
# avg_max = sum(max_learner) / len(max_learner)
# max_max = max(max_learner)
# print(f"average max: {avg_max}")
# print(f"maximum max score: {max_max}")
# print()
#
max_learner = []

for i in range(0, 100):
    agent = LearnerOptimized(alpha=0.1, epsilon=0.005)
    hist = []
    run_games(agent, hist, 100, 100)
    max_learner.append(max(hist))

print("optimized learner:")

avg_max = sum(max_learner) / len(max_learner)
max_max = max(max_learner)
print(f"average max: {avg_max}")
print(f"maximum max score: {max_max}")
print()


def mean_maxes(epsilon=0.01, alpha=0.5):
    epochs = 100

    hists = []
    for i in range(0, 100):
        agent = LearnerOptimized(epsilon=epsilon, alpha=alpha)
        hist = []
        run_games(agent, hist, epochs, 0)
        hists.append(hist)

    max_hists = [
        [max(hists[i][:n]) for n in range(1, epochs)] for i in range(0, len(hists))
    ]

    return [
        np.mean([max_hists[i][n] for i in range(0, len(hists))])
        for n in range(0, epochs - 1)
    ]


#

import matplotlib.pyplot as plt

# a = mean_maxes(epsilon=0.005)
# b = mean_maxes(epsilon=0.01)
# c = mean_maxes(epsilon=0.05)
# d = mean_maxes(epsilon=0.1)
#
# plt.figure(figsize=(10, 5))
# plt.plot(a, label="epsilon=0.005")
# plt.plot(b, label="epsilon=0.01")
# plt.plot(c, label="epsilon=0.05")
# plt.plot(d, label="epsilon=0.1")
# plt.title("Mean Max Score vs Epoch")
# plt.xlabel("Epoch")
# plt.ylabel("Mean Max Score")
# plt.legend()
# plt.savefig("figures/epsilon_graph.pdf")
# plt.show()
#
# a = mean_maxes(alpha=0.01)
# b = mean_maxes(alpha=0.05)
# c = mean_maxes(alpha=0.1)
# d = mean_maxes(alpha=0.5)
#
# plt.figure(figsize=(10, 5))
# plt.plot(a, label="alpha=0.01")
# plt.plot(b, label="alpha=0.05")
# plt.plot(c, label="alpha=0.1")
# plt.plot(d, label="alpha=0.5")
# plt.title("Mean Max Score vs Epoch")
# plt.xlabel("Epoch")
# plt.ylabel("Mean Max Score")
# plt.legend()
# plt.savefig("figures/alpha_graph.pdf")
# plt.show()
