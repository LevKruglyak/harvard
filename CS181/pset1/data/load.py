import numpy as np


def training_data():
    train_data = np.genfromtxt(
        "data/earth_temperature_sampled_train.csv", delimiter=","
    )

    year_train = train_data[:, 0] / 1000
    temp_train = train_data[:, 1]

    return (year_train, temp_train)


def testing_data():
    test_data = np.genfromtxt("data/earth_temperature_sampled_test.csv", delimiter=",")

    year_test = test_data[:, 0] / 1000
    temp_test = test_data[:, 1]

    return (year_test, temp_test)
