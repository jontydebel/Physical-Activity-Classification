import glob
import os
import pandas as pd
import numpy as np
import numpy.lib.stride_tricks as slide

path = "./data"
csv_files = glob.glob(os.path.join(path, "*.csv"))
num_samples = 60
processed_data = np.array(["accel_mean", "gyro_mean", "accel_stddev", "gyro_stddev", "z_mean", "z_stddev", "activity"])

for filepath in csv_files:
    # ingest dataset
    data = pd.read_csv(filepath, header=None)
    data = data.to_numpy()

    # vectorize
    accel_magnitude = np.sqrt(data[:, 0] ** 2 + data[:, 1] ** 2 + data[:, 2] ** 2)
    gyro_magnitude = np.sqrt(data[:, 3] ** 2 + data[:, 4] ** 2 + data[:, 5] ** 2)
    z_axis = data[:, 2]

    # sliding window
    windowed_accel = slide.sliding_window_view(accel_magnitude, num_samples)
    windowed_gyro = slide.sliding_window_view(gyro_magnitude, num_samples)
    windowed_z_accel = slide.sliding_window_view(z_axis, num_samples)

    # derive properties
    accel_mean = windowed_accel.mean(axis=1)
    gyro_mean = windowed_gyro.mean(axis=1)
    accel_stddev = windowed_accel.std(axis=1)
    gyro_stddev = windowed_gyro.std(axis=1)
    z_mean = windowed_z_accel.mean(axis=1)
    z_stddev = windowed_z_accel.std(axis=1)
    activity = np.full_like(gyro_stddev, str(filepath)[7:-6], dtype=object)
    batch = np.stack((accel_mean, gyro_mean, accel_stddev, gyro_stddev, z_mean, z_stddev, activity), axis=1)
    processed_data = np.vstack((processed_data, batch))
processed_data = pd.DataFrame(processed_data)
processed_data.to_csv("./_PROCESSED.csv", header=None, index=None)
print("Done")