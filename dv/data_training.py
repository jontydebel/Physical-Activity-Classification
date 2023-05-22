import pandas as pd
import numpy as np
import pickle
# import matplotlib.pyplot as plt
import os
import glob
from sklearn.neighbors import KNeighborsClassifier


def main():
    print("Starting now")
    data = pd.read_csv("_PROCESSED.csv")
    data = np.array(data)

    rows, cols = np.shape(data)
    index = np.random.choice(rows, round(rows * 0.8), replace=False)
    training_batch = data[index, :]
    test_batch = np.delete(data, index, 0)
    test_input = test_batch[:, 0:6]
    true_output = test_batch[:, 6]

    # for x in range(1,5):
    #     print(f"Test {x}")
    #     for j in range(185,195,1):
    #         model = train_KNN_classifier(j, training_batch)
    #         # print(f"Testing with {j*50} neighbours")
    #         test_rows, test_cols = np.shape(test_batch)
    #         correct = 0
    #         for i in range(1000):
    #
    #             row_idx = np.random.randint(low=0, high=test_rows)
    #             result = predict_movement(model, test_input[row_idx, :])
    #             if result[0] == true_output[row_idx]:
    #                 correct += 1
    #             # else:
    #                 # print(f"INCORRECT - Predicted: {result[0]}, Actual: {true_output[row_idx]}")
    #         print(f"{j} Neightbours: {correct} out of 1000 correct")

    model = train_KNN_classifier(190, training_batch)
    pickle.dump(model, open("Model_KNN.obj", "wb"))
    # model = pickle.load(open("Model_KNN.obj", "rb"))

    test_rows, test_cols = np.shape(test_batch)

    correct = 0
    for i in range(1000):
        row_idx = np.random.randint(low=0, high=test_rows)
        result = predict_movement(model, test_input[row_idx, :])
        if result[0] == true_output[row_idx]:
            correct += 1
        else:
            print(f"INCORRECT - Predicted: {result[0]}, Actual: {true_output[row_idx]}")
    print(f"{correct} out of 1000 correct")


def train_KNN_classifier(neighbours, training_data):
    classifier = KNeighborsClassifier(n_neighbors=neighbours)

    # split data into X,y where X is features and y is result
    X = training_data[:, 0:6]
    y = training_data[:, 6]

    classifier.fit(X, y)
    return classifier


def predict_movement(classifier, input):
    input_array = np.array(input, dtype=float).reshape((1, -1))
    output = classifier.predict(input_array)
    return output


# main()
