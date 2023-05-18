import requests
import time
import threading
import data_training
import serial
import serial.tools.list_ports as st
from strip_ansi import strip_ansi
import json
import pickle
import numpy as np

max_pos = 4
min_pos = 0
ser = 0


def main():

    # ser = init_serial()
    file = open("Model_KNN.obj", "rb")
    model = pickle.load(file)
    file.close()

    accel_gyro_data = []
    while True:
        data_line = serial_readline()
        accel_gyro_row = []
        headings = ["accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z"]
        try:
            for key in headings:
                accel_gyro_row.append(data_line[key])
            accel_gyro_data.append(accel_gyro_row)
            if len(accel_gyro_data) > 60:
                accel_gyro_data = accel_gyro_data[-60:]
        except Exception:
            print("passed")
            continue

        transformed_data = transform_data(accel_gyro_data)

        movement_class = data_training.predict_movement(model, transformed_data)

        # worker_thread = threading.Thread(target=send_to_dashboard, args=(data_line, position))
        # worker_thread.start()
        time.sleep(9.2)
        print(threading.active_count())


def init_serial():
    # ======== CONNECT ==========
    try:
        ports = st.comports()
        name = ""

        for port in ports:
            print(f"device:{port.device} - name:{port.name} - description:{port.description} - hwid:{port.hwid}")
            hwid = port.hwid[12:16]
            if hwid == "C553":
                name = port.name
                print("selected " + name)
                break

        # name = "/dev/" + name
        ser = serial.Serial(name, 115200)
        return ser

    except Exception:
        time.sleep(1)
        init_serial()


def transform_data(accel_gyro_raw):
    raw_data = np.array(accel_gyro_raw, dtype=float)

    # Vectorization
    accel_magnitude = np.sqrt(raw_data[:, 0] ** 2 + raw_data[:, 1] ** 2 + raw_data[:, 2] ** 2)
    gyro_magnitude = np.sqrt(raw_data[:, 3] ** 2 + raw_data[:, 4] ** 2 + raw_data[:, 5] ** 2)

    # Aggregation
    accel_mean = accel_magnitude.mean()
    accel_stddev = accel_magnitude.std()
    gyro_mean = gyro_magnitude.mean()
    gyro_stddev = gyro_magnitude.std()

    return [accel_mean, gyro_mean, accel_stddev, gyro_stddev]


def serial_readline():
    global ser
    try:
        ser.flushInput()
        dataline = ser.readline().decode().strip().replace(" ", "")
    except Exception:
        print("Reconnecting...")
        ser = init_serial()
    try:
        dataline = strip_ansi(dataline)
        dataline = dataline.replace("CSSE4011:~$", "")
        dataline = dataline[dataline.find("{"):]
        print(dataline)
        dataline_json = json.loads(dataline)
        return dataline_json
    except Exception:
        print("Failed to parse JSON")
        return


def send_to_dashboard(rssi_values: dict, position: tuple):
    token = "1265c444-e54c-4fd8-80b1-ea7423c26075"
    headers = {"Authorization": token}
    test_data = []
    for node, value in rssi_values.items():
        test_data.append({"variable": node, "value": value})

    x_pos, y_pos = position
    test_data.append({"variable": "x_pos", "value": round(x_pos, 2)})
    test_data.append({"variable": "y_pos", "value": round(y_pos, 2)})

    if x_pos < min_pos:
        x_pos = 0.1
    elif x_pos > max_pos:
        x_pos = 3.99

    if y_pos < min_pos:
        y_pos = 0.1
    elif y_pos > max_pos:
        y_pos = 3.99

    test_data.append({"variable": "position", "value": 1,
                      "metadata": {"x": x_pos / max_pos, "y": y_pos / max_pos}})

    response = requests.post("https://api.tago.io/data", headers=headers, json=test_data)

    print(response.text)


main()
