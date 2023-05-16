import pandas as pd
import numpy as np
# import matplotlib.pyplot as plt
import os
import glob
from sklearn.neighbors import KNeighborsRegressor


def load_and_train_regressor(neighbours):
    path = "./otherData"
    csv_files = glob.glob(os.path.join(path, "*.csv"))

    data = []
    for file in csv_files:
        temp = pd.read_csv(file, index_col=None)
        data.append(temp)

    data = pd.concat(data, ignore_index=True, axis=0)
    data = data.drop_duplicates()
    data = np.array(data)

    regressor = KNeighborsRegressor(n_neighbors=neighbours)

    # split data into X,y where X is features and y is result
    y = data[:, 8:10]
    X = data[:, 0:8]

    regressor.fit(X, y)
    return regressor


def predict_pos(regressor, input):
    input_array = np.array(input, dtype=float).reshape(1, -1)
    output = regressor.predict(input_array)
    return tuple(output[0])

# main()