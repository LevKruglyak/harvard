import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.colors as c
import matplotlib.patches as mpatches
from scipy.special import expit as sigmoid
from scipy.special import softmax
from scipy.special import logsumexp
from scipy.stats import multivariate_normal


def basis1(t):
    return np.stack([np.ones(len(t)), t], axis=1)


def basis2(t):
    return np.stack([np.ones(len(t)), t, np.power(t, 2)], axis=1)


def basis3(t):
    return np.stack(
        [
            np.ones(len(t)),
            t,
            np.power(t, 2),
            np.power(t, 3),
            np.power(t, 4),
            np.power(t, 5),
        ],
        axis=1,
    )


class LogisticRegressor:
    def __init__(self, eta, runs):
        self.eta = eta
        self.runs = runs

    def fit(self, x, y, w_init):
        """
        Optimize the weights W to minimize the negative log-likelihood by using gradient descent

        :param x: a 2D numpy array of transformed feature values. Shape is (n x 2), (n x 3), or (n x 6)
        :param y: a 2D numpy array of output values. Shape is (n x 1)
        :param w_init: a 2D numpy array that initializes the weights. Shape is (d x 1), where d is the number of dimensions of transformed feature values.
        :return: None
        """
        self.W = w_init

        for _ in range(self.runs):
            predictions = sigmoid(np.dot(x, self.W))
            errors = predictions - y
            gradients = np.dot(x.T, errors) / x.shape[0]
            self.W -= self.eta * gradients

    def predict(self, x):
        """
        Predict classification probability of transformed input x

        :param x: a 2D numpy array of transformed feature values. Shape is (n x 2), (n x 3), or (n x 6)
        :return: a 2D numpy array of predicted probabilities given current weights. Shape should be (n x 1)
        """
        x = np.asarray(x)
        return sigmoid(np.dot(x, self.W))


class GaussianGenerativeModel:
    def __init__(self, is_shared_covariance=False):
        self.is_shared_covariance = is_shared_covariance
        self.classes = 3

    def fit(self, X, y):
        """
        Compute the means and (shared) covariance matrix of the data. Compute the prior over y.

        :param X: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :param y: a 1D numpy array of target values (Dwarf=0, Giant=1, Supergiant=2).
        :return: None
        """
        self.priors = [np.sum(y == C) / len(y) for C in range(self.classes)]
        self.means = [np.mean(X[y == C], axis=0) for C in range(self.classes)]

        # Compute covariance matrix
        mat = np.array(
            [
                np.outer(X[i] - self.means[y[i]], X[i] - self.means[y[i]])
                for i in range(len(X))
            ]
        )
        covs = [np.sum(mat[y == i], axis=0) for i in range(self.classes)]

        if self.is_shared_covariance:
            self.cov = np.sum(covs, axis=0) / len(y)
        else:
            self.cov = covs / np.bincount(y).reshape(-1, 1, 1)

    def likelihood(self, x, type):
        if self.is_shared_covariance:
            return self.priors[type] * multivariate_normal.pdf(
                x, mean=self.means[type], cov=self.cov
            )
        else:
            return self.priors[type] * multivariate_normal.pdf(
                x, mean=self.means[type], cov=self.cov[type]
            )

    def predict(self, X_pred):
        """
        The code in this method should be removed and replaced! We included it
        just so that the distribution code is runnable and produces a
        (currently meaningless) visualization.

        Predict classes of points given feature values in X_pred

        :param X_pred: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :return: a 1D numpy array of predicted classes (Dwarf=0, Giant=1, Supergiant=2).
                 Shape should be (n,)
        """
        preds = np.zeros(len(X_pred))
        for i in range(len(X_pred)):
            probs = np.array([self.likelihood(X_pred[i], j) for j in range(3)])
            preds[i] = np.argmax(probs)

        return preds

    def negative_log_likelihood(self, X, y):
        """
        Given the data X, use previously calculated class means and covariance matrix to
        calculate the negative log likelihood of the data
        """

        # Calculate the negative log likelihood of the data
        return -np.sum(np.log([self.likelihood(X[i], y[i]) for i in range(len(y))]))


import numpy as np


class SoftmaxRegression:
    def __init__(self, eta, lam, bias=True, runs=200000):
        self.eta = eta
        self.lam = lam
        self.runs = runs
        self.bias = bias
        self.W = None

    def add_bias(self, X):
        return np.hstack([np.ones((len(X), 1)), X])

    def fit(self, X, y):
        """
        Fit the weights W of softmax regression using gradient descent with L2 regularization
        Use the results from Problem 2 to find an expression for the gradient

        :param X: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :param y: a 1D numpy array of target values (Dwarf=0, Giant=1, Supergiant=2).
        :return: None
        """
        # Include the bias term
        if self.bias:
            X = self.add_bias(X)

        # Initializing the weights (do not change!)
        # The number of classes is 1 + (the highest numbered class)
        num_classes = 1 + y.max()
        num_features = X.shape[1]
        self.W = np.ones((num_classes, num_features))

        # Matrix of indicator variables for gradient calculation
        indicators = np.zeros((len(X), 3))
        for i in range(len(X)):
            indicators[i, y[i]] = 1

        for _ in range(self.runs):
            # Matrix of probabilities
            predictions = softmax(np.matmul(X, self.W.T), axis=1)

            # Compute the regularized gradient
            gradient = np.matmul((predictions - indicators).T, X) + self.lam * self.W

            # Update the weights
            self.W -= self.eta * gradient

    def predict(self, X_pred):
        """
        The code in this method should be removed and replaced! We included it
        just so that the distribution code is runnable and produces a
        (currently meaningless) visualization.

        Predict classes of points given feature values in X_pred

        :param X_pred: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :return: a 1D numpy array of predicted classes (Dwarf=0, Giant=1, Supergiant=2).
                 Shape should be (n,)
        """
        # TODO
        # Calculate the probability array for each class
        probs = self.predict_proba(X_pred)
        # Choose the classes with the highest probability
        preds = np.argmax(probs, axis=1)

        return preds

    def predict_proba(self, X_pred):
        """
        Predict classification probabilities of points given feature values in X_pred

        :param X_pred: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :return: a 2D numpy array of predicted class probabilities (Dwarf=index 0, Giant=index 1, Supergiant=index 2).
                 Shape should be (n x 3)
        """
        # TODO
        # Include the bias term
        if self.bias:
            X_pred = self.add_bias(X_pred)

        # Calculate the probability array for each class
        return softmax(np.matmul(X_pred, self.W.T), axis=1)


class KNNClassifier:
    def __init__(self, k):
        self.X = None
        self.y = None
        self.K = k

    def fit(self, X, y):
        """
        In KNN, "fitting" can be as simple as storing the data, so this has been written for you.
        If you'd like to add some preprocessing here without changing the inputs, feel free,
        but this is completely optional.
        """
        self.X = X
        self.y = y

    def distance(self, a, b):
        return (a[0] - b[0]) ** 2 / 9 + (a[1] - b[1]) ** 2

    def predict(self, X_pred):
        """
        The code in this method should be removed and replaced! We included it
        just so that the distribution code is runnable and produces a
        (currently meaningless) visualization.

        Predict classes of points given feature values in X_pred

        :param X_pred: a 2D numpy array of (transformed) feature values. Shape is (n x 2)
        :return: a 1D numpy array of predicted classes (Dwarf=0, Giant=1, Supergiant=2).
                 Shape should be (n,)
        """
        preds = np.zeros(len(X_pred))
        for i in range(len(X_pred)):
            # Calculate the distance to each point in the training set
            distances = np.array(
                [self.distance(X_pred[i], self.X[j]) for j in range(len(self.X))]
            )

            # Find the k nearest neighbors
            nearest = np.argsort(distances)[: self.K]

            # Find the most common class among the k nearest neighbors
            classes = self.y[nearest]
            counts = np.bincount(classes, minlength=3)
            preds[i] = np.argmax(counts)
        return preds

    # Quickly coded for part 2
    def predict_proba(self, X_pred):
        counts = np.zeros((len(X_pred), 3))
        for i in range(len(X_pred)):
            # Calculate the distance to each point in the training set
            distances = np.array(
                [self.distance(X_pred[i], self.X[j]) for j in range(len(self.X))]
            )

            # Find the k nearest neighbors
            nearest = np.argsort(distances)[: self.K]

            # Find the most common class among the k nearest neighbors
            classes = self.y[nearest]
            counts[i] = np.bincount(classes, minlength=3)
            return counts
